#include "TransientLimiter.h"
#include <juce_audio_basics/juce_audio_basics.h>
#include <cmath>
#include <algorithm>
#include <numeric>

static constexpr float kMaxLookaheadMs = 5.0f;
static constexpr float kMinGain        = 1e-6f;   // -120 dB floor
static constexpr float kEpsilon        = 1e-9f;

// ---------------------------------------------------------------------------
// dB helpers — release smoothing operates in dB domain so gain reduction
// decreases linearly in dB per unit time (exponential decay in linear domain).
// ---------------------------------------------------------------------------
static inline float gainToDecibels(float linearGain)
{
    return 20.0f * std::log10(std::max(linearGain, kMinGain));
}

static inline float decibelsToGain(float dB)
{
    return std::pow(10.0f, dB * (1.0f / 20.0f));
}

// ---------------------------------------------------------------------------
// prepare
// ---------------------------------------------------------------------------
void TransientLimiter::prepare(double sampleRate, int /*maxBlockSize*/, int numChannels)
{
    mSampleRate   = sampleRate;
    mNumChannels  = numChannels;

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
// setThreshold
// ---------------------------------------------------------------------------
void TransientLimiter::setThreshold(float linear)
{
    mThreshold = std::clamp(linear, 1e-6f, 1.0f);
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
    const float releaseMs = 10.0f + params.releaseShape * 490.0f;
    mReleaseCoeff = std::exp(-1.0f / (releaseMs * 0.001f * static_cast<float>(mSampleRate)));
}

// ---------------------------------------------------------------------------
// getGainReduction
// ---------------------------------------------------------------------------
float TransientLimiter::getGainReduction() const
{
    return mCurrentGRdB;
}

// ---------------------------------------------------------------------------
// computeRequiredGain  (private)
// ---------------------------------------------------------------------------
float TransientLimiter::computeRequiredGain(float peakAbs) const
{
    if (peakAbs < kEpsilon)
        return 1.0f;

    const float kneeHalf = mParams.kneeWidth * 0.5f;

    if (kneeHalf < 0.01f)
    {
        // Hard knee
        return (peakAbs > mThreshold) ? (mThreshold / peakAbs) : 1.0f;
    }

    // Convert to dB for soft-knee calculation
    const float peakDb  = 20.0f * std::log10(peakAbs);
    const float threshDb = 20.0f * std::log10(mThreshold);
    const float lowerDb  = threshDb - kneeHalf;
    const float upperDb  = threshDb + kneeHalf;

    if (peakDb <= lowerDb)
        return 1.0f;  // below knee — no reduction

    if (peakDb >= upperDb)
    {
        // Above knee — full limiting
        return mThreshold / peakAbs;
    }

    // Within knee: quadratic interpolation of gain in dB
    const float t = (peakDb - lowerDb) / mParams.kneeWidth;  // 0–1 across knee
    // At t=0: 0 dB gain change; at t=1: (threshDb - peakDb) dB gain change
    const float gainDb = (threshDb - upperDb) * t * t;
    return std::pow(10.0f, gainDb / 20.0f);
}

// ---------------------------------------------------------------------------
// softSaturate  (private, static)
// ---------------------------------------------------------------------------
float TransientLimiter::softSaturate(float x, float amount)
{
    if (amount < 0.001f)
        return x;

    // Drive > 1 increases soft-clipping effect; blend with dry by (1-amount)
    const float drive = 1.0f + amount * 3.0f;
    const float wet   = std::tanh(x * drive) / drive;
    return x * (1.0f - amount) + wet * amount;
}

// ---------------------------------------------------------------------------
// process
// ---------------------------------------------------------------------------
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
        // --- 1. Write input (and sidechain) into delay buffers + update sliding max ---
        for (int ch = 0; ch < chCount; ++ch)
        {
            if (!bypassDelay)
            {
                // Main path: write sample and update monotone deque
                const float mainAbs = std::abs(channelData[ch][s]);
                mDelayBuffers[ch][mWritePos[ch]] = channelData[ch][s];
                mWritePos[ch] = (mWritePos[ch] + 1) % bufSize;

                SWDeque& md = mMainDeques[ch];
                int&     mc = mMainWriteCount[ch];
                while (!md.empty() && md.back().value <= mainAbs)
                    md.pop_back();
                md.push_back({mainAbs, mc});
                while (!md.empty() && md.front().pos <= mc - lookahead)
                    md.pop_front();
                ++mc;

                if (sidechainData != nullptr)
                {
                    const float scAbs = std::abs(sidechainData[ch][s]);
                    mSidechainDelayBuffers[ch][mSidechainWritePos[ch]] = sidechainData[ch][s];
                    mSidechainWritePos[ch] = (mSidechainWritePos[ch] + 1) % bufSize;

                    SWDeque& sd = mSCDeques[ch];
                    int&     sc = mSCWriteCount[ch];
                    while (!sd.empty() && sd.back().value <= scAbs)
                        sd.pop_back();
                    sd.push_back({scAbs, sc});
                    while (!sd.empty() && sd.front().pos <= sc - lookahead)
                        sd.pop_front();
                    ++sc;
                }
            }
        }

        // --- 2. Find per-channel peak in lookahead window ----------------------
        //        When sidechain provided, read sidechain deque maximum.
        //        Otherwise read main deque maximum.
        //        For bypass path: just use current input sample directly.
        float perChRequiredGain[8];  // supports up to 8 channels (7.1 surround)
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
        if (chCount > 1 && mChannelLink > 0.0f)
        {
            float minRequired = 1.0f;
            for (int ch = 0; ch < chCount; ++ch)
                minRequired = std::min(minRequired, perChRequiredGain[ch]);

            for (int ch = 0; ch < chCount; ++ch)
                perChRequiredGain[ch] = perChRequiredGain[ch] * (1.0f - mChannelLink)
                                        + minRequired * mChannelLink;
        }

        // --- 4. Smooth gain: instant attack, exponential release ---------------
        // Release smoothing is done in dB domain so that the gain reduction
        // decreases at a constant dB/time rate (linear in dB = exponential in
        // linear). Smoothing in linear domain would produce non-uniform dB
        // decay and audible pumping artifacts.
        for (int ch = 0; ch < chCount; ++ch)
        {
            float& g = mGainState[ch];
            const float target = perChRequiredGain[ch];

            if (target < g)
            {
                g = target;  // instant attack
            }
            else
            {
                // Release: interpolate in dB domain toward target (usually 0 dB)
                const float gDb      = gainToDecibels(g);
                const float targetDb = gainToDecibels(target);
                const float smoothedDb = gDb + (targetDb - gDb) * (1.0f - mReleaseCoeff);
                g = decibelsToGain(smoothedDb);
            }

            g = std::clamp(g, kMinGain, 1.0f);
        }

        // Optionally apply adaptive release: when gain is dropping fast, shorten
        // release proportionally to the rate of change.
        // (This is handled by the instant-attack policy above.)

        // --- 5. Read delayed sample and apply gain + saturation ----------------
        for (int ch = 0; ch < chCount; ++ch)
        {
            float delayed;
            if (bypassDelay)
            {
                delayed = channelData[ch][s];
            }
            else
            {
                // Read position: lookahead samples behind writePos
                // writePos was already incremented above, so the oldest
                // buffered sample (lookahead samples ago) is at:
                const int readPos = (mWritePos[ch] - lookahead + bufSize) % bufSize;
                delayed = mDelayBuffers[ch][readPos];
            }

            float out = delayed * mGainState[ch];
            out = softSaturate(out, mParams.saturationAmount);
            channelData[ch][s] = out;

            if (mGainState[ch] < minGain)
                minGain = mGainState[ch];
        }
    }

    // Update reported GR (convert minimum gain this block to dB)
    mCurrentGRdB = (minGain < 1.0f)
                       ? 20.0f * std::log10(std::max(minGain, kMinGain))
                       : 0.0f;
}
