#include "TransientLimiter.h"
#include "DspUtil.h"
#include <juce_audio_basics/juce_audio_basics.h>
#include <juce_dsp/juce_dsp.h>
#include <cmath>
#include <algorithm>
#include <numeric>

static constexpr float kMaxLookaheadMs = 5.0f;
static constexpr float kEpsilon        = 1e-9f;

// Release time range for the shape-controlled release envelope.
// releaseShape 0 → kReleaseMinMs, releaseShape 1 → kReleaseMaxMs.
static constexpr float kReleaseMinMs = 10.0f;
static constexpr float kReleaseMaxMs = 500.0f;

// ---------------------------------------------------------------------------
// prepare
// ---------------------------------------------------------------------------
void TransientLimiter::prepare(double sampleRate, int /*maxBlockSize*/, int numChannels)
{
    mSampleRate   = sampleRate;
    jassert (numChannels <= kMaxChannels);
    mNumChannels  = std::min (numChannels, kMaxChannels);

    // Allocate delay buffers for maximum possible lookahead
    mMaxLookaheadSamples = static_cast<int>(kMaxLookaheadMs * 0.001 * sampleRate) + 1;

    mDelayBuffers.assign(numChannels, std::vector<float>(mMaxLookaheadSamples + 1, 0.0f));
    mWritePos.assign(numChannels, 0);
    mGainState.assign(numChannels, 1.0f);

    mSidechainDelayBuffers.assign(numChannels, std::vector<float>(mMaxLookaheadSamples + 1, 0.0f));
    mSidechainWritePos.assign(numChannels, 0);

    // Pre-allocate sliding-window maximum deques (no audio-thread heap allocs)
    const int deqCap = mMaxLookaheadSamples + 1;
    mMainDeques.resize(numChannels);
    mSCDeques.resize(numChannels);
    for (int ch = 0; ch < numChannels; ++ch)
    {
        mMainDeques[ch].reserve(deqCap);
        mSCDeques[ch].reserve(deqCap);
    }
    mMainWriteCount.assign(numChannels, 0);
    mSCWriteCount.assign(numChannels, 0);

    // Default release: ~50 ms
    const float releaseMs = 50.0f;
    mReleaseCoeff = std::exp(-1.0f / (releaseMs * 0.001f * static_cast<float>(sampleRate)));

    mCurrentGRdB = 0.0f;

    // Apply current params to update release coeff
    setAlgorithmParams(mParams);
}

// ---------------------------------------------------------------------------
// setLookahead
// ---------------------------------------------------------------------------
void TransientLimiter::setLookahead(float ms)
{
    const float clamped = std::clamp(ms, 0.0f, kMaxLookaheadMs);
    mLookaheadSamples = static_cast<int>(clamped * 0.001f * static_cast<float>(mSampleRate));

    // Clamp to allocated buffer size
    if (mLookaheadSamples > mMaxLookaheadSamples)
        mLookaheadSamples = mMaxLookaheadSamples;
}

// ---------------------------------------------------------------------------
// updateKneeCache  (private)
// ---------------------------------------------------------------------------
void TransientLimiter::updateKneeCache()
{
    mThresholdDb           = gainToDecibels(mThreshold);
    const float kneeHalf   = mParams.kneeWidth * 0.5f;
    mLowerKneeDb           = mThresholdDb - kneeHalf;
    mUpperKneeDb           = mThresholdDb + kneeHalf;
    mLinearLowerKneeThresh = decibelsToGain(mLowerKneeDb);
}

// ---------------------------------------------------------------------------
// setThreshold
// ---------------------------------------------------------------------------
void TransientLimiter::setThreshold(float linear)
{
    mThreshold = clampThreshold(linear);
    updateKneeCache();
}

// ---------------------------------------------------------------------------
// setChannelLink
// ---------------------------------------------------------------------------
void TransientLimiter::setChannelLink(float pct)
{
    mChannelLink = std::clamp(pct, 0.0f, 1.0f);
}

// ---------------------------------------------------------------------------
// setAlgorithmParams
// ---------------------------------------------------------------------------
void TransientLimiter::setAlgorithmParams(const AlgorithmParams& params)
{
    mParams = params;

    // Map releaseShape (0–1) to a release time in ms (10–500 ms range)
    // Higher releaseShape = longer release (smoother/more transparent)
    const float releaseMs = kReleaseMinMs + params.releaseShape * (kReleaseMaxMs - kReleaseMinMs);
    mReleaseCoeff = std::exp(-1.0f / (releaseMs * 0.001f * static_cast<float>(mSampleRate)));

    updateKneeCache();
}

// ---------------------------------------------------------------------------
// getGainReduction
// ---------------------------------------------------------------------------
float TransientLimiter::getGainReduction() const
{
    return mCurrentGRdB;
}

// ---------------------------------------------------------------------------
// getLatencyInSamples
// ---------------------------------------------------------------------------
int TransientLimiter::getLatencyInSamples() const
{
    return mLookaheadSamples;
}

// ---------------------------------------------------------------------------
// resetCounters
// ---------------------------------------------------------------------------
void TransientLimiter::resetCounters(int64_t startOffset)
{
    for (int ch = 0; ch < mNumChannels; ++ch)
    {
        mMainWriteCount[ch] = startOffset;
        mSCWriteCount[ch]   = startOffset;
        mMainDeques[ch].reset();
        mSCDeques[ch].reset();
    }

    // Clear gain and GR state so no GR carries over after reset
    std::fill(mGainState.begin(), mGainState.end(), 1.0f);
    mCurrentGRdB = 0.0f;

    // Clear lookahead delay buffers
    for (auto& buf : mDelayBuffers)
        std::fill(buf.begin(), buf.end(), 0.0f);

    // Reset write/read positions
    std::fill(mWritePos.begin(), mWritePos.end(), 0);
}

// ---------------------------------------------------------------------------
// computeRequiredGain  (private)
// ---------------------------------------------------------------------------
/**
 * @brief Compute the linear gain factor needed to keep @p peakAbs at or below threshold.
 *
 * @details Three operating regions based on the soft-knee width:
 *
 *   - **Below knee** (peakDb ≤ threshDb − kneeHalf): return 1.0 (no reduction).
 *   - **Within knee** (threshDb − kneeHalf < peakDb < threshDb + kneeHalf):
 *     quadratic interpolation in dB — gain change ramps from 0 dB at the lower
 *     knee edge to full limiting at the upper edge:
 *       t = (peakDb − lowerDb) / kneeWidth   [0..1 across knee]
 *       gainDb = (threshDb − upperDb) × t²
 *     This produces a smooth onset that avoids the hard transient of a brick-wall
 *     limiter (AES17 / typical mastering limiter convention).
 *   - **Above knee** (peakDb ≥ threshDb + kneeHalf): hard limiting,
 *     return threshold / peakAbs.
 *
 *   When kneeWidth < 0.01 the soft-knee path is skipped and a simple
 *   threshold-over-peak ratio is used (hard knee, avoids log/pow overhead).
 *
 * @return Linear gain in (0, 1]. Returns 1.0 when @p peakAbs < kEpsilon.
 */
float TransientLimiter::computeRequiredGain(float peakAbs) const
{
    if (peakAbs < kEpsilon)
        return 1.0f;

    const float kneeHalf = mParams.kneeWidth * 0.5f;

    if (kneeHalf < 0.01f)
    {
        // Hard knee — no log10 needed
        return (peakAbs > mThreshold) ? (mThreshold / peakAbs) : 1.0f;
    }

    // Linear-domain early exit: below lower knee → no reduction (no log10 call).
    // mLinearLowerKneeThresh is pre-computed by updateKneeCache().
    if (peakAbs <= mLinearLowerKneeThresh)
        return 1.0f;

    // At this point we are in or above the knee — log10 is justified.
    const float peakDb = gainToDecibels(peakAbs);

    if (peakDb >= mUpperKneeDb)
    {
        // Above knee — hard limit
        return mThreshold / peakAbs;
    }

    // Within knee: quadratic interpolation of gain in dB
    const float t      = (peakDb - mLowerKneeDb) / mParams.kneeWidth;  // 0–1 across knee
    const float gainDb = (mThresholdDb - mUpperKneeDb) * t * t;
    return decibelsToGain(gainDb);
}

// ---------------------------------------------------------------------------
// softSaturate  (private, static)
// ---------------------------------------------------------------------------
float TransientLimiter::softSaturate(float x, float amount)
{
    if (amount < 0.001f)
        return x;

    // Drive > 1 increases soft-clipping effect; blend with dry by (1-amount).
    // Normalize so f(1.0) == 1.0: divide by tanh(drive), not drive.
    const float drive = 1.0f + amount * 3.0f;
    const float wet   = std::tanh(x * drive) / std::tanh(drive);
    return x * (1.0f - amount) + wet * amount;
}

// ---------------------------------------------------------------------------
// process
// ---------------------------------------------------------------------------
/**
 * @brief Process audio in-place using lookahead peak limiting.
 *
 * @details Algorithm per sample:
 *   1. Write input sample into a circular lookahead delay buffer of length N
 *      (N = mLookaheadSamples). The write happens AFTER the read so the delay
 *      is exactly N samples, not N-1.
 *   2. Maintain a per-channel sliding-window maximum deque (O(1) amortised)
 *      over the past N+1 samples to find the window peak without scanning.
 *   3. Compute required gain from the window peak via computeRequiredGain()
 *      (applies soft-knee curve if kneeWidth > 0).
 *   4. Apply channel linking: blend each channel's required gain toward the
 *      minimum across all channels (mChannelLink=1 fully links, 0 is independent).
 *   5. Smooth gain: instant attack (target < current → snap), exponential
 *      release in linear domain (avoids per-sample transcendental calls at the
 *      cost of <0.5% deviation from dB-domain smoothing for typical coefficients).
 *   6. Read delayed sample, apply smoothed gain, apply soft saturation (tanh),
 *      then write original input into the delay buffer.
 *
 * When @p sidechainData is non-null, peak detection uses the sidechain signal
 * (which passes through its own parallel delay buffer to stay time-aligned)
 * while gain is applied to @p channelData.
 *
 * When lookahead == 0, the delay path is bypassed and limiting is applied
 * in-place with zero latency.
 *
 * @note No heap allocation occurs in this function. All buffers are pre-allocated
 *       in prepare(). Thread safety: call only from the audio thread.
 */
void TransientLimiter::process(float** channelData, int numChannels, int numSamples,
                                const float* const* sidechainData)
{
    juce::ScopedNoDenormals noDenormals;
    if (mDelayBuffers.empty() || numChannels <= 0 || numSamples <= 0)
        return;

    const int chCount   = std::min(numChannels, mNumChannels);
    const int bufSize   = mMaxLookaheadSamples + 1;
    const int lookahead = mLookaheadSamples;

    // When lookahead == 0, bypass the delay path and limit in-place
    const bool bypassDelay = (lookahead == 0);

    float minGain = 1.0f;  // track minimum gain this block for GR reporting

    for (int s = 0; s < numSamples; ++s)
    {
        // --- 1. Update sliding-max deques with the current input sample.
        //        Delay buffer writes happen AFTER the read in Step 5 so that
        //        the read position is taken before the write pointer advances,
        //        giving an exact lookahead-sample delay (not lookahead-1). ---
        for (int ch = 0; ch < chCount; ++ch)
        {
            if (!bypassDelay)
            {
                // Main path: update monotone deque only (no buffer write yet)
                const float mainAbs = std::abs(channelData[ch][s]);

                SWDeque&   md = mMainDeques[ch];
                int64_t&   mc = mMainWriteCount[ch];
                while (!md.empty() && md.back().value <= mainAbs)
                    md.pop_back();
                md.push_back({mainAbs, mc});
                // With read-before-write the delay is exactly N samples.
                // The detection window must include position mc-lookahead so
                // that a peak detected now is still "visible" when its delayed
                // copy reaches the output exactly lookahead steps later.
                // Use strict less-than (<) instead of <= so the window spans
                // [mc - lookahead, mc] (N+1 entries).
                while (!md.empty() && md.front().pos < mc - lookahead)
                    md.pop_front();
                ++mc;

                if (sidechainData != nullptr)
                {
                    const float scAbs = std::abs(sidechainData[ch][s]);

                    SWDeque&  sd = mSCDeques[ch];
                    int64_t&  sc = mSCWriteCount[ch];
                    while (!sd.empty() && sd.back().value <= scAbs)
                        sd.pop_back();
                    sd.push_back({scAbs, sc});
                    while (!sd.empty() && sd.front().pos < sc - lookahead)
                        sd.pop_front();
                    ++sc;
                }
            }
        }

        // --- 2. Find per-channel peak in lookahead window ----------------------
        //        When sidechain provided, read sidechain deque maximum.
        //        Otherwise read main deque maximum.
        //        For bypass path: just use current input sample directly.
        float perChRequiredGain[kMaxChannels];
        for (int ch = 0; ch < chCount; ++ch)
            perChRequiredGain[ch] = 1.0f;

        for (int ch = 0; ch < chCount; ++ch)
        {
            float peakAbs;

            if (bypassDelay)
            {
                const float* detectCh = (sidechainData != nullptr) ? sidechainData[ch] : channelData[ch];
                peakAbs = std::abs(detectCh[s]);
            }
            else
            {
                // O(1) lookup: front of the monotone deque is the window maximum
                SWDeque& deq = (sidechainData != nullptr) ? mSCDeques[ch] : mMainDeques[ch];
                peakAbs = deq.empty() ? 0.0f : deq.front().value;
            }

            perChRequiredGain[ch] = computeRequiredGain(peakAbs);
        }

        // --- 3. Channel linking ------------------------------------------------
        //  At mChannelLink=1.0: all channels share the minimum required gain.
        //  At mChannelLink=0.0: each channel is independent.
        applyChannelLinking(perChRequiredGain, chCount, mChannelLink);

        // --- 4. Smooth gain: instant attack, exponential release ---------------
        // Release smoothing is done in the linear domain using a one-pole IIR:
        //   g_new = g * alpha + target * (1 - alpha)
        // For typical release coefficients (alpha in [0.97, 1.0]), the per-step
        // difference from dB-domain smoothing is < 0.5%, which is inaudible.
        // This avoids two transcendental calls (log10 + pow) per sample per
        // channel in the release branch, which at 32x oversampling would
        // execute millions of times per second.
        //
        // SIMD path (stereo only): both channels are processed simultaneously
        // using a branchless formulation with SSE/NEON 4-wide float SIMD.
        // Instant-attack fast path (transientAttackCoeff >= 0.999) remains as a
        // scalar pre-check to avoid SIMD overhead in that common edge case.
#if JUCE_USE_SIMD
        using SIMDf     = juce::dsp::SIMDRegister<float>;
        using vMaskType = typename SIMDf::vMaskType;
        using MaskType  = typename SIMDf::MaskType;
        if (chCount == 2 && SIMDf::SIMDNumElements >= 4
            && mParams.transientAttackCoeff < 0.999f)
        {
            // Load current gain states and targets into the low two lanes.
            // We use a 4-wide register; lanes 2 and 3 are unused (set to 1.0
            // for gain / 0 for any difference, so they don't corrupt the clamp).
            alignas(SIMDf::SIMDNumElements * sizeof(float)) float gArr[4]  =
                { mGainState[0], mGainState[1], 1.0f, 1.0f };
            alignas(SIMDf::SIMDNumElements * sizeof(float)) float tArr[4]  =
                { perChRequiredGain[0], perChRequiredGain[1], 1.0f, 1.0f };

            SIMDf g      = SIMDf::fromRawArray(gArr);
            SIMDf target = SIMDf::fromRawArray(tArr);

            const float alpha       = mParams.transientAttackCoeff;
            const float releaseCoef = mReleaseCoeff;

            // Attack: g*(1-alpha) + target*alpha
            SIMDf attackGain  = g * SIMDf::expand(1.0f - alpha)
                              + target * SIMDf::expand(alpha);
            // Release: g*releaseCoef + target*(1-releaseCoef)
            SIMDf releaseGain = g * SIMDf::expand(releaseCoef)
                              + target * SIMDf::expand(1.0f - releaseCoef);

            // Branchless blend: use attack where target < g, else release.
            // Technique: mask out each term and ADD the results.
            // Where mask=0xFFFFFFFF: attack is preserved, release is zeroed.
            // Where mask=0x00000000: attack is zeroed, release is preserved.
            // The two sets are disjoint so addition = bitwise-OR.
            vMaskType useAttack  = SIMDf::lessThan(target, g);
            vMaskType notAttack  = useAttack ^ vMaskType::expand(MaskType(0xFFFFFFFFu));

            // Cast vMaskType back to float-domain via SIMDf::operator&(vMaskType)
            SIMDf newG = (attackGain  & useAttack)
                       + (releaseGain & notAttack);

            // Clamp to [kDspUtilMinGain, 1.0f]
            newG = SIMDf::max(newG, SIMDf::expand(kDspUtilMinGain));
            newG = SIMDf::min(newG, SIMDf::expand(1.0f));

            alignas(SIMDf::SIMDNumElements * sizeof(float)) float outArr[4];
            newG.copyToRawArray(outArr);
            mGainState[0] = outArr[0];
            mGainState[1] = outArr[1];
        }
        else
#endif // JUCE_USE_SIMD
        {
            // Scalar path: used for mono/surround, instant-attack, or
            // platforms without SIMD support.
            for (int ch = 0; ch < chCount; ++ch)
            {
                float& g = mGainState[ch];
                const float target = perChRequiredGain[ch];

                if (target < g)
                {
                    if (mParams.transientAttackCoeff >= 0.999f)
                    {
                        g = target;  // instant attack at coefficient = 1
                    }
                    else
                    {
                        // Interpolate: 0 = very slow IIR snap, 1 = instant snap
                        const float alpha = mParams.transientAttackCoeff;
                        g = g * (1.0f - alpha) + target * alpha;
                    }
                }
                else
                {
                    // Release: linear-domain IIR toward target (usually 1.0, i.e. 0 dB)
                    g = g * mReleaseCoeff + target * (1.0f - mReleaseCoeff);
                }

                g = std::clamp(g, kDspUtilMinGain, 1.0f);
            }
        }

        // Optionally apply adaptive release: when gain is dropping fast, shorten
        // release proportionally to the rate of change.
        // (This is handled by the instant-attack policy above.)

        // --- 5. Read delayed sample, apply gain + saturation, then write input ---
        //        Read-before-write ordering: mWritePos[ch] has not been
        //        incremented yet, so the sample written exactly lookahead
        //        steps ago is at position (mWritePos - lookahead), giving a
        //        true delay of exactly lookaheadSamples (not lookahead-1).
        for (int ch = 0; ch < chCount; ++ch)
        {
            // Save the original input before it is overwritten with the output.
            const float inputSample = channelData[ch][s];

            float delayed;
            if (bypassDelay)
            {
                delayed = channelData[ch][s];
            }
            else
            {
                // Read BEFORE advancing writePos so offset is exact.
                const int readPos = (mWritePos[ch] + bufSize - lookahead) % bufSize;
                delayed = mDelayBuffers[ch][readPos];
            }

            float out = delayed * mGainState[ch];
            out = softSaturate(out, mParams.saturationAmount);
            channelData[ch][s] = out;

            // Write the original input into the delay buffer after the read.
            if (!bypassDelay)
            {
                mDelayBuffers[ch][mWritePos[ch]] = inputSample;
                mWritePos[ch] = (mWritePos[ch] + 1) % bufSize;

                if (sidechainData != nullptr)
                {
                    mSidechainDelayBuffers[ch][mSidechainWritePos[ch]] = sidechainData[ch][s];
                    mSidechainWritePos[ch] = (mSidechainWritePos[ch] + 1) % bufSize;
                }
            }

            if (mGainState[ch] < minGain)
                minGain = mGainState[ch];
        }
    }

    // Update reported GR (convert minimum gain this block to dB)
    mCurrentGRdB = (minGain < 1.0f)
                       ? gainToDecibels(std::max(minGain, kDspUtilMinGain))
                       : 0.0f;
}
