#include "LevelingLimiter.h"
#include <juce_audio_basics/juce_audio_basics.h>
#include <cmath>
#include <algorithm>

static constexpr float kMinGaindB          = -120.0f; // dB floor for gain state
static constexpr float kMinGain            = 1e-6f;   // linear equivalent of kMinGaindB
static constexpr float kEpsilon            = 1e-9f;
static constexpr float kAdaptiveSmoothMs   = 500.0f;  // time constant for adaptive release detection

// ---------------------------------------------------------------------------
// dB helpers — the envelope follower operates in the dB domain so that
// attack/release coefficients produce correct exponential dB curves.
// Smoothing in linear domain would give wrong (non-exponential) envelope
// shapes and audible pumping artifacts.
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
void LevelingLimiter::prepare(double sampleRate, int /*maxBlockSize*/, int numChannels)
{
    mSampleRate  = sampleRate;
    mNumChannels = numChannels;

    mGainState.assign(numChannels, 0.0f);  // dB domain: 0 dB = no reduction
    mEnvState.assign(numChannels, 0.0f);   // dB domain: 0 dB = no sustained GR

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
    juce::ScopedNoDenormals noDenormals;
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

        // --- 3. Smooth gain with attack and release (dB domain) --------------
        // Operating in dB domain ensures that attack/release coefficients
        // produce constant dB/time rates (linear in dB = exponential in
        // linear domain). mGainState[ch] holds the current gain in dB
        // (0 = no reduction, negative = reducing).
        for (int ch = 0; ch < chCount; ++ch)
        {
            float& gDb = mGainState[ch];  // dB domain state
            const float targetDb = gainToDecibels(perChRequiredGain[ch]);

            // Update long-term envelope tracker for adaptive release (dB domain).
            // mEnvState drifts negative when sustained gain reduction is present.
            mEnvState[ch] = mEnvState[ch] * mAdaptiveSmoothCoeff
                            + targetDb * (1.0f - mAdaptiveSmoothCoeff);

            if (targetDb < gDb)
            {
                // Attack: more reduction needed — approach target in dB domain.
                // attackCoeff = 0 → instant attack.
                gDb = gDb * mAttackCoeff + targetDb * (1.0f - mAttackCoeff);
            }
            else
            {
                // Release: recovering toward 0 dB.
                float effectiveReleaseCoeff = mReleaseCoeff;

                if (mParams.adaptiveRelease)
                {
                    // sustainedGRdB > 0 indicates heavy, sustained gain reduction.
                    const float sustainedGRdB = -mEnvState[ch];

                    if (sustainedGRdB > 0.5f)
                    {
                        // Speed up release proportionally to sustained GR depth.
                        const float speedup = std::min(1.0f, sustainedGRdB / 6.0f);
                        // faster coeff = coeff^2 (compounding exponential)
                        const float fastCoeff = mReleaseCoeff * mReleaseCoeff;
                        effectiveReleaseCoeff = mReleaseCoeff * (1.0f - speedup)
                                                + fastCoeff * speedup;
                    }
                }

                // Exponential recovery in dB: gDb → 0 at effective release rate
                gDb = gDb + (0.0f - gDb) * (1.0f - effectiveReleaseCoeff);
            }

            gDb = std::max(gDb, kMinGaindB);
        }

        // --- 4. Apply gain to main audio -------------------------------------
        for (int ch = 0; ch < chCount; ++ch)
        {
            const float linearGain = decibelsToGain(mGainState[ch]);
            channelData[ch][s] *= linearGain;

            if (linearGain < minGain)
                minGain = linearGain;
        }
    }

    mCurrentGRdB = (minGain < 1.0f)
                       ? gainToDecibels(minGain)
                       : 0.0f;
}
