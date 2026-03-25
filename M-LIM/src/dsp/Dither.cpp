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
        // Nyquist gain: |N(-1)| = |1 + c1 - c2| = 3.62 (~22 dB above white noise), pushing
        // quantisation noise energy well above 10 kHz where the ear is less sensitive.
        mCoeff1 = 1.80f;
        mCoeff2 = -0.82f;
    }
    else
    {
        // 44.1 kHz range: near-DC-null second-order noise shaping.
        // Choose c2 = -(|z|^2) = -0.91 so pole magnitude |z| = sqrt(0.91) ≈ 0.954 < 1 (strictly stable).
        // c1 = 1 + c2 + ε ≈ 1.90 gives near-DC-null: N(1) = 1 - c1 - c2 = 0.01 (-40 dB attenuation).
        // Nyquist gain: |N(-1)| = |1 + c1 - c2| = 3.81 (~22 dB above white noise), pushing
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

        // Pre-quantisation value: input + dither + noise-shaping feedback.
        float v = data[i] + dither;

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

        // Quantise to nearest step.
        const float quantized = std::round(v / step) * step;

        // Update error history for noise shaping.
        const float error = quantized - v;
        mError2 = mError1;
        mError1 = error;

        data[i] = quantized;
    }
}
