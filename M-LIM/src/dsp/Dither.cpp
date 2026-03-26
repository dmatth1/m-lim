#include "Dither.h"
#include <juce_audio_basics/juce_audio_basics.h>
#include <cmath>

void Dither::prepare(double sampleRate)
{
    mSampleRate = sampleRate;

    // Weighted mode (mode 2) uses F-weighted second-order error feedback.
    // Coefficients are tuned per sample rate; at >=88.2kHz the shaped noise
    // would be ultrasonic, so coefficients are zeroed (falls back to Basic behavior).
    if (sampleRate >= 88200.0)
    {
        // Shaped noise frequencies exceed Nyquist/2 — disable second-order feedback.
        mCoeff1 = 0.0f;
        mCoeff2 = 0.0f;
    }
    else if (sampleRate >= 47000.0)
    {
        // 48 kHz range: near-DC-null second-order noise shaping.
        // Choose c2 = -(|z|^2) = -0.82 so pole magnitude |z| = sqrt(0.82) ≈ 0.906 < 1 (strictly stable).
        // c1 = 1 + c2 + ε ≈ 1.80 gives near-DC-null: N(1) = 1 - c1 - c2 = 0.02 (-34 dB attenuation).
        // Nyquist gain: |N(-1)| = |1 + c1 - c2| = 3.62 (~11 dB above white noise floor), pushing
        // quantisation noise energy well above 10 kHz where the ear is less sensitive.
        mCoeff1 = 1.80f;
        mCoeff2 = -0.82f;
    }
    else
    {
        // 44.1 kHz range: near-DC-null second-order noise shaping.
        // Choose c2 = -(|z|^2) = -0.91 so pole magnitude |z| = sqrt(0.91) ≈ 0.954 < 1 (strictly stable).
        // c1 = 1 + c2 + ε ≈ 1.90 gives near-DC-null: N(1) = 1 - c1 - c2 = 0.01 (-40 dB attenuation).
        // Nyquist gain: |N(-1)| = |1 + c1 - c2| = 3.81 (~12 dB above white noise floor), pushing
        // quantisation noise energy well above 10 kHz where the ear is less sensitive.
        mCoeff1 = 1.90f;
        mCoeff2 = -0.91f;
    }

    mError1 = 0.0f;
    mError2 = 0.0f;
}

void Dither::setBitDepth(int bits)
{
    mBitDepth = bits;
    // Step size for a normalised [-1, 1] float signal.
    // With B bits, we have 2^B levels over [−1, 1], so step = 2^(1 − B).
    mStepSize = std::pow(2.0f, 1.0f - static_cast<float>(bits));

    mError1 = 0.0f;
    mError2 = 0.0f;
}

void Dither::setNoiseShaping(int mode)
{
    mNoiseShaping = mode;
    mError1 = 0.0f;
    mError2 = 0.0f;
}

/**
 * @brief Quantize samples in-place to the target bit depth with TPDF dither and optional noise shaping.
 *
 * @details Per sample:
 *   1. **TPDF dither**: two independent uniform random values each in [-step/2, +step/2]
 *      are summed to produce a triangular distribution over [-step, +step] with zero mean
 *      and variance = step²/6 (1/3 LSB² RMS). TPDF dither decorrelates quantisation error
 *      from the signal and eliminates granulation noise (AES17 / Vanderkooy & Lipshitz 1984).
 *   2. **Noise shaping** (if enabled): quantisation error from previous samples is fed back
 *      as a pre-quantisation correction to push noise energy to higher frequencies:
 *      - Mode 1 (first-order): v -= e[n-1]   (c1 = 1, flat correction)
 *      - Mode 2 (second-order): v -= c1*e[n-1] + c2*e[n-2]  (rate- and sample-rate-tuned)
 *        Coefficients are chosen per sample rate in prepare() to give near-DC-null response
 *        and push excess noise energy above 10 kHz where the ear is less sensitive.
 *   3. **Quantise**: round(v / step) × step — maps to the nearest quantisation level.
 *   4. **Error update**: e[n] = quantized - v, shifted into the two-sample history.
 *
 * @note Mode 0 (Basic): TPDF only, no error feedback. Step size = 2^(1−B) for B-bit depth.
 * @see  Vanderkooy & Lipshitz, "Resolution below the Least Significant Bit", JAES 1984
 */
void Dither::process(float* data, int numSamples)
{
    juce::ScopedNoDenormals noDenormals;
    const float step = mStepSize;

    for (int i = 0; i < numSamples; ++i)
    {
        // TPDF dither: two independent uniform random values each in
        // [-step/2, +step/2], summed to produce a triangular distribution
        // over [-step, +step] with zero mean and 2-LSB peak-to-peak.
        const float r1 = (mRandom.nextFloat() - 0.5f) * step;
        const float r2 = (mRandom.nextFloat() - 0.5f) * step;
        const float dither = r1 + r2;

        // Pre-quantisation value: input + noise-shaping feedback (no dither yet).
        // Dither is added to the pre-quantisation value but must NOT be included
        // in the error signal — per Vanderkooy & Lipshitz 1984 / AES17.
        float v = data[i];

        if (mNoiseShaping == 1)
        {
            // First-order: feed back last quantisation error (c1 = 1).
            v -= mError1;
        }
        else if (mNoiseShaping == 2)
        {
            // Second-order: weighted feedback tuned for current sample rate.
            v -= mCoeff1 * mError1 + mCoeff2 * mError2;
        }

        // Quantise to nearest step, with dither added at quantisation time.
        const float quantized = std::round((v + dither) / step) * step;

        // Error is relative to the undithered+feedback value.
        // e[n] = Q(v + d) - v  (dither is not corrected by feedback)
        const float error = quantized - v;
        mError2 = mError1;
        mError1 = error;

        data[i] = quantized;
    }
}
