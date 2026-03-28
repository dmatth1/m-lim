#include "LevelingLimiter.h"
#include "DspUtil.h"
#include <juce_audio_basics/juce_audio_basics.h>
#include <juce_dsp/juce_dsp.h>
#include <cmath>
#include <algorithm>

static constexpr float kMinGaindB          = -120.0f; // dB floor for gain state
static constexpr float kMinGain            = 1e-6f;   // linear equivalent of kMinGaindB
static constexpr float kEpsilon            = 1e-9f;
static constexpr float kAdaptiveSmoothMs   = 500.0f;  // time constant for adaptive release detection

// ---------------------------------------------------------------------------
// prepare
// ---------------------------------------------------------------------------
void LevelingLimiter::prepare(double sampleRate, int /*maxBlockSize*/, int numChannels)
{
    mSampleRate  = sampleRate;
    mNumChannels = numChannels;

    mGainState.assign(numChannels, 1.0f);  // linear domain: 1.0 = no reduction
    mEnvState.assign(numChannels, 1.0f);   // linear domain: 1.0 = no sustained GR

    updateCoefficients();

    mCurrentGRdB = 0.0f;
    mCurrentMinGainLinear = 1.0f;
}

// ---------------------------------------------------------------------------
// reset
// ---------------------------------------------------------------------------
void LevelingLimiter::reset()
{
    std::fill(mGainState.begin(), mGainState.end(), 1.0f);
    std::fill(mEnvState.begin(),  mEnvState.end(),  1.0f);
    mCurrentGRdB = 0.0f;
    mCurrentMinGainLinear = 1.0f;
}

// ---------------------------------------------------------------------------
// updateCoefficients  (private)
// ---------------------------------------------------------------------------
void LevelingLimiter::updateCoefficients()
{
    const float sr = static_cast<float>(mSampleRate);

    // Attack coefficient: near 0 → instant, near 1 → very slow
    mAttackCoeff = (mAttackMs > 0.01f)
                       ? std::exp(-1.0f / (mAttackMs * 0.001f * sr))
                       : 0.0f;

    // Release coefficient
    mReleaseCoeff = std::exp(-1.0f / (mReleaseMs * 0.001f * sr));

    // Slow adaptive smoother
    mAdaptiveSmoothCoeff = std::exp(-1.0f / (kAdaptiveSmoothMs * 0.001f * sr));
}

// ---------------------------------------------------------------------------
// setThreshold
// ---------------------------------------------------------------------------
void LevelingLimiter::setThreshold(float linear)
{
    mThreshold = clampThreshold(linear);
}

// ---------------------------------------------------------------------------
// setAttack
// ---------------------------------------------------------------------------
void LevelingLimiter::setAttack(float ms)
{
    const float clamped = std::clamp(ms, 0.0f, 100.0f);
    if (clamped == mAttackMs)
        return;
    mAttackMs = clamped;
    updateCoefficients();
}

// ---------------------------------------------------------------------------
// setRelease
// ---------------------------------------------------------------------------
void LevelingLimiter::setRelease(float ms)
{
    const float clamped = std::clamp(ms, 10.0f, 1000.0f);
    if (clamped == mReleaseMs)
        return;
    mReleaseMs = clamped;
    updateCoefficients();
}

// ---------------------------------------------------------------------------
// setChannelLink
// ---------------------------------------------------------------------------
void LevelingLimiter::setChannelLink(float pct)
{
    mChannelLink = std::clamp(pct, 0.0f, 1.0f);
}

// ---------------------------------------------------------------------------
// setAlgorithmParams
// ---------------------------------------------------------------------------
void LevelingLimiter::setAlgorithmParams(const AlgorithmParams& params)
{
    mParams = params;
}

// ---------------------------------------------------------------------------
// getGainReduction
// ---------------------------------------------------------------------------
float LevelingLimiter::getGainReduction() const
{
    return mCurrentGRdB;
}

// ---------------------------------------------------------------------------
// getMinGainLinear
// ---------------------------------------------------------------------------
float LevelingLimiter::getMinGainLinear() const
{
    return mCurrentMinGainLinear;
}

// ---------------------------------------------------------------------------
// computeRequiredGain  (private)
// ---------------------------------------------------------------------------
float LevelingLimiter::computeRequiredGain(float peakAbs) const
{
    if (peakAbs < kEpsilon)
        return 1.0f;

    return (peakAbs > mThreshold) ? (mThreshold / peakAbs) : 1.0f;
}

// ---------------------------------------------------------------------------
// process
// ---------------------------------------------------------------------------
/**
 * @brief Process audio in-place using a slow release envelope follower.
 *
 * @details Stage 2 of the dual-stage limiter. Unlike Stage 1 (TransientLimiter),
 * there is no lookahead delay — this stage shapes the release envelope to provide
 * gentle, program-dependent level control after fast peaks have been caught.
 *
 * Algorithm per sample:
 *   1. Detect per-channel peak from main audio (post-Stage-1).
 *   2. Compute required gain: threshold / peak, or 1.0 if below threshold.
 *   3. Apply channel linking: blend per-channel gains toward the minimum.
 *   4. Smooth gain with a first-order IIR in linear domain:
 *      - **Attack** (more reduction needed): g = g * attackCoeff + target * (1 − attackCoeff).
 *        Slower than Stage 1's instant attack to avoid pumping on dense program material.
 *      - **Release** (recovering toward unity): g = g * releaseCoeff + (1 − releaseCoeff).
 *   5. Optional adaptive release: if sustained GR is deep (measured via a
 *      ~500 ms slow envelope in mEnvState), the release coefficient is computed
 *      only once per block (not per sample) via squaring for compounding speedup.
 *   6. Apply gain to main audio in-place.
 *
 * All time coefficients are in linear domain to avoid per-sample log/pow calls.
 * Deviation from dB-domain smoothing is < 0.5% for typical attack/release times.
 *
 * @note No heap allocation occurs in this function. Thread safety: audio thread only.
 */
void LevelingLimiter::process(float** channelData, int numChannels, int numSamples)
{
    juce::ScopedNoDenormals noDenormals;
    if (mGainState.empty() || numChannels <= 0 || numSamples <= 0)
        return;

    const int chCount = std::min(numChannels, mNumChannels);
    float minGain = 1.0f;

    // Stack-allocated per-channel required gain (supports up to 8 channels)
    float perChRequiredGain[8];

    // Pre-compute effective release coefficients per channel (once per block).
    // Adaptive speedup uses mEnvState which is in linear domain; gainToDecibels()
    // is called here (once per channel) rather than per sample.
    float effectiveReleaseCoeffs[8];
    for (int ch = 0; ch < chCount; ++ch)
    {
        effectiveReleaseCoeffs[ch] = mReleaseCoeff;
        if (mParams.adaptiveRelease)
        {
            // mEnvState is in linear domain: 1.0 = no reduction, <1 = reduction.
            // sustainedGRdB > 0 indicates sustained gain reduction.
            const float sustainedGRdB = -gainToDecibels(mEnvState[ch]);

            if (sustainedGRdB > 0.5f)
            {
                // Speed up release proportionally to sustained GR depth.
                const float speedup = std::min(1.0f, sustainedGRdB / 6.0f);
                // faster coeff = coeff^2 (compounding exponential)
                const float fastCoeff = mReleaseCoeff * mReleaseCoeff;
                effectiveReleaseCoeffs[ch] = mReleaseCoeff * (1.0f - speedup)
                                             + fastCoeff * speedup;
            }
        }
    }

    for (int s = 0; s < numSamples; ++s)
    {
        // --- 1. Compute required gain per channel ----------------------------
        for (int ch = 0; ch < chCount; ++ch)
        {
            perChRequiredGain[ch] = computeRequiredGain(std::abs(channelData[ch][s]));
        }

        // --- 2. Channel linking -----------------------------------------------
        applyChannelLinking(perChRequiredGain, chCount, mChannelLink);

        // --- 3. Smooth gain in linear domain ---------------------------------
        // Steps 3a (adaptive envelope) must remain scalar since it uses a single
        // mAdaptiveSmoothCoeff. Steps 3b (attack/release) and 4 (gain apply) get
        // a SIMD fast path for stereo.

        // 3a. Update long-term envelope tracker (always scalar).
        if (mParams.adaptiveRelease)
        {
            for (int ch = 0; ch < chCount; ++ch)
            {
                const float target = perChRequiredGain[ch];
                mEnvState[ch] = mEnvState[ch] * mAdaptiveSmoothCoeff
                                + target * (1.0f - mAdaptiveSmoothCoeff);
            }
        }

        // 3b. Attack/release smoothing + 4. gain apply.
#if JUCE_USE_SIMD
        using SIMDf     = juce::dsp::SIMDRegister<float>;
        using vMaskType = typename SIMDf::vMaskType;
        using MaskType  = typename SIMDf::MaskType;
        if (chCount == 2 && SIMDf::SIMDNumElements >= 4)
        {
            // Load gain states and targets into SIMD lanes (lanes 2-3 = 1.0).
            alignas(SIMDf::SIMDNumElements * sizeof(float)) float gArr[4] =
                { mGainState[0], mGainState[1], 1.0f, 1.0f };
            alignas(SIMDf::SIMDNumElements * sizeof(float)) float tArr[4] =
                { perChRequiredGain[0], perChRequiredGain[1], 1.0f, 1.0f };

            SIMDf g      = SIMDf::fromRawArray(gArr);
            SIMDf target = SIMDf::fromRawArray(tArr);

            // Attack: g * attackCoeff + target * (1 - attackCoeff)
            SIMDf attackGain = g * SIMDf::expand(mAttackCoeff)
                             + target * SIMDf::expand(1.0f - mAttackCoeff);

            // Release uses per-channel coefficients loaded into SIMD lanes.
            // Release target is unity (1.0), not `target`.
            alignas(SIMDf::SIMDNumElements * sizeof(float)) float relArr[4] =
                { effectiveReleaseCoeffs[0], effectiveReleaseCoeffs[1], 1.0f, 1.0f };
            alignas(SIMDf::SIMDNumElements * sizeof(float)) float relInvArr[4] =
                { 1.0f - effectiveReleaseCoeffs[0], 1.0f - effectiveReleaseCoeffs[1], 0.0f, 0.0f };
            SIMDf relCoeff    = SIMDf::fromRawArray(relArr);
            SIMDf relCoeffInv = SIMDf::fromRawArray(relInvArr);

            // Release: g * relCoeff + (1 - relCoeff)  [targets unity]
            SIMDf releaseGain = g * relCoeff + relCoeffInv;

            // Branchless blend: attack where target < g, else release.
            vMaskType useAttack = SIMDf::lessThan(target, g);
            vMaskType notAttack = useAttack ^ vMaskType::expand(MaskType(0xFFFFFFFFu));

            SIMDf newG = (attackGain & useAttack) + (releaseGain & notAttack);

            // Clamp to [kMinGain, 1.0f]
            newG = SIMDf::max(newG, SIMDf::expand(kMinGain));
            newG = SIMDf::min(newG, SIMDf::expand(1.0f));

            alignas(SIMDf::SIMDNumElements * sizeof(float)) float outArr[4];
            newG.copyToRawArray(outArr);
            mGainState[0] = outArr[0];
            mGainState[1] = outArr[1];

            // Apply gain to main audio and track minimum.
            channelData[0][s] *= mGainState[0];
            channelData[1][s] *= mGainState[1];

            const float localMin = std::min(mGainState[0], mGainState[1]);
            if (localMin < minGain)
                minGain = localMin;
        }
        else
#endif // JUCE_USE_SIMD
        {
            // Scalar path: mono, surround, or platforms without SIMD.
            for (int ch = 0; ch < chCount; ++ch)
            {
                float& g = mGainState[ch];
                const float target = perChRequiredGain[ch];

                if (target < g)
                {
                    // Attack: more reduction needed.
                    g = g * mAttackCoeff + target * (1.0f - mAttackCoeff);
                }
                else
                {
                    // Release: recovering toward unity (1.0).
                    g = g * effectiveReleaseCoeffs[ch] + (1.0f - effectiveReleaseCoeffs[ch]);
                }

                g = std::max(g, kMinGain);
            }

            // Apply gain to main audio.
            for (int ch = 0; ch < chCount; ++ch)
            {
                const float linearGain = mGainState[ch];
                channelData[ch][s] *= linearGain;

                if (linearGain < minGain)
                    minGain = linearGain;
            }
        }
    }

    mCurrentMinGainLinear = std::max(minGain, kDspUtilMinGain);
    mCurrentGRdB = (minGain < 1.0f)
                       ? gainToDecibels(mCurrentMinGainLinear)
                       : 0.0f;
}
