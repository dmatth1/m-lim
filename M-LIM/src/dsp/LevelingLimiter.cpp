#include "LevelingLimiter.h"
#include <cmath>
#include <algorithm>

static constexpr float kMinGain            = 1e-6f;   // -120 dB floor
static constexpr float kEpsilon            = 1e-9f;
static constexpr float kAdaptiveSmoothMs   = 500.0f;  // time constant for adaptive release detection

// ---------------------------------------------------------------------------
// prepare
// ---------------------------------------------------------------------------
void LevelingLimiter::prepare(double sampleRate, int /*maxBlockSize*/, int numChannels)
{
    mSampleRate  = sampleRate;
    mNumChannels = numChannels;

    mGainState.assign(numChannels, 1.0f);
    mEnvState.assign(numChannels, 1.0f);

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
    if (mGainState.empty() || numChannels <= 0 || numSamples <= 0)
        return;

    const int chCount = std::min(numChannels, mNumChannels);
    float minGain = 1.0f;

    // Stack-allocated per-channel required gain (supports up to 8 channels)
    float perChRequiredGain[8];

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
        if (chCount > 1 && mChannelLink > 0.0f)
        {
            float minRequired = 1.0f;
            for (int ch = 0; ch < chCount; ++ch)
                minRequired = std::min(minRequired, perChRequiredGain[ch]);

            for (int ch = 0; ch < chCount; ++ch)
                perChRequiredGain[ch] = perChRequiredGain[ch] * (1.0f - mChannelLink)
                                        + minRequired * mChannelLink;
        }

        // --- 3. Smooth gain with attack and release --------------------------
        for (int ch = 0; ch < chCount; ++ch)
        {
            float& g = mGainState[ch];
            const float target = perChRequiredGain[ch];

            // Update long-term envelope tracker for adaptive release
            // mEnvState follows the required gain with a slow time constant.
            // When sustained gain reduction occurs, mEnvState drifts below 1.
            mEnvState[ch] = mEnvState[ch] * mAdaptiveSmoothCoeff
                            + target * (1.0f - mAdaptiveSmoothCoeff);

            if (target < g)
            {
                // Attack: gain is being reduced — approach target exponentially.
                // For near-zero attack time the coefficient is 0 → instant.
                g = g * mAttackCoeff + target * (1.0f - mAttackCoeff);
            }
            else
            {
                // Release: gain is recovering toward unity.
                float effectiveReleaseCoeff = mReleaseCoeff;

                if (mParams.adaptiveRelease)
                {
                    // How much gain reduction has been sustained on average?
                    // mEnvState close to 1 = little sustained GR; near 0 = heavy sustained GR.
                    const float sustainedGR = 1.0f - mEnvState[ch];

                    if (sustainedGR > 0.05f)
                    {
                        // Speed up release proportionally to the sustained GR depth.
                        // Interpolate between normal coeff and a faster one.
                        const float speedup = std::min(1.0f, sustainedGR * 10.0f);
                        // faster coeff = coeff^2 (compounding exponential)
                        const float fastCoeff = mReleaseCoeff * mReleaseCoeff;
                        effectiveReleaseCoeff = mReleaseCoeff * (1.0f - speedup)
                                                + fastCoeff * speedup;
                    }
                }

                // Exponential recovery: g → 1 at the effective release rate
                g = g + (1.0f - g) * (1.0f - effectiveReleaseCoeff);
            }

            g = std::clamp(g, kMinGain, 1.0f);
        }

        // --- 4. Apply gain to main audio -------------------------------------
        for (int ch = 0; ch < chCount; ++ch)
        {
            channelData[ch][s] *= mGainState[ch];

            if (mGainState[ch] < minGain)
                minGain = mGainState[ch];
        }
    }

    mCurrentGRdB = (minGain < 1.0f)
                       ? 20.0f * std::log10(std::max(minGain, kMinGain))
                       : 0.0f;
}
