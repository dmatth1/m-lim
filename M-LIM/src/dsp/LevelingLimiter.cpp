#include "LevelingLimiter.h"
#include "DspUtil.h"
#include <juce_audio_basics/juce_audio_basics.h>
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
    mAttackMs = std::clamp(ms, 0.0f, 100.0f);
    updateCoefficients();
}

// ---------------------------------------------------------------------------
// setRelease
// ---------------------------------------------------------------------------
void LevelingLimiter::setRelease(float ms)
{
    mReleaseMs = std::clamp(ms, 10.0f, 1000.0f);
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
void LevelingLimiter::process(float** channelData, int numChannels, int numSamples,
                               const float* const* sidechainData)
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
            const float* detectCh = (sidechainData != nullptr)
                                        ? sidechainData[ch]
                                        : channelData[ch];
            perChRequiredGain[ch] = computeRequiredGain(std::abs(detectCh[s]));
        }

        // --- 2. Channel linking -----------------------------------------------
        applyChannelLinking(perChRequiredGain, chCount, mChannelLink);

        // --- 3. Smooth gain in linear domain ---------------------------------
        // mGainState[ch] is in linear scale (1.0 = no reduction, <1 = reducing).
        // Tracking in linear domain avoids per-sample transcendental calls while
        // remaining numerically close to dB-domain smoothing for typical
        // attack/release times (>10 ms), where per-sample delta is <0.5%.
        for (int ch = 0; ch < chCount; ++ch)
        {
            float& g = mGainState[ch];  // linear domain state
            const float target = perChRequiredGain[ch];  // already linear

            // Update long-term envelope tracker for adaptive release.
            // mEnvState is in linear domain (1.0 = no reduction, <1 = reduction).
            if (mParams.adaptiveRelease)
            {
                mEnvState[ch] = mEnvState[ch] * mAdaptiveSmoothCoeff
                                + target * (1.0f - mAdaptiveSmoothCoeff);
            }

            if (target < g)
            {
                // Attack: more reduction needed — first-order IIR in linear domain.
                g = g * mAttackCoeff + target * (1.0f - mAttackCoeff);
            }
            else
            {
                // Release: recovering toward unity (1.0) in linear domain.
                g = g * effectiveReleaseCoeffs[ch] + (1.0f - effectiveReleaseCoeffs[ch]);
            }

            g = std::max(g, kMinGain);
        }

        // --- 4. Apply gain to main audio -------------------------------------
        for (int ch = 0; ch < chCount; ++ch)
        {
            const float linearGain = mGainState[ch];  // already linear, no conversion needed
            channelData[ch][s] *= linearGain;

            if (linearGain < minGain)
                minGain = linearGain;
        }
    }

    mCurrentGRdB = (minGain < 1.0f)
                       ? gainToDecibels(minGain)
                       : 0.0f;
}
