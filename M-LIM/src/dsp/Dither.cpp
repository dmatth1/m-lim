#include "Dither.h"
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
        // 48 kHz range: slightly adjusted F-weighted coefficients.
        mCoeff1 = 0.95f;
        mCoeff2 = -0.95f;
    }
    else
    {
        // 44.1 kHz range: standard F-weighted second-order coefficients.
        mCoeff1 = 1.0f;
        mCoeff2 = -1.0f;
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
