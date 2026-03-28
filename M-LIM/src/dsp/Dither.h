#pragma once

#include <juce_core/juce_core.h>

/**
 * Dither — TPDF dithering with optional noise shaping.
 *
 * Adds triangular probability density function (TPDF) dither noise before
 * quantizing to the target bit depth. Three noise shaping modes control
 * how quantization error is fed back to push noise energy to higher frequencies.
 *
 * Noise shaping modes:
 *   0 = Basic     — TPDF dither only, no error feedback
 *   1 = Optimized — first-order error feedback (c1 = 1)
 *   2 = Weighted  — second-order feedback tuned to sample rate
 */
class Dither
{
public:
    Dither() = default;

    /** Call before processing or when sample rate changes.
     *  Selects noise shaping coefficients and resets error state.
     *  For stream reset without reconfiguring, call reset() instead. */
    void prepare(double sampleRate);

    /** Clear runtime state (error history) without recomputing coefficients.
     *  Use this for stream resets when sample rate hasn't changed. */
    void reset() noexcept;

    /** Process samples in-place, quantizing to target bit depth with dither. */
    void process(float* data, int numSamples);

    /** Set quantization bit depth: 16, 18, 20, 22, or 24. */
    void setBitDepth(int bits);

    /** Set noise shaping mode: 0=Basic, 1=Optimized, 2=Weighted. */
    void setNoiseShaping(int mode);

private:
    double mSampleRate   = 44100.0;
    int    mBitDepth     = 24;
    int    mNoiseShaping = 0;

    float mStepSize = 1.0f;   // quantisation step in normalised [-1, 1] range
    float mError1   = 0.0f;   // quantisation error one sample ago
    float mError2   = 0.0f;   // quantisation error two samples ago

    float mCoeff1 = 1.4f;     // noise-shaping feedback coefficient 1
    float mCoeff2 = -0.5f;    // noise-shaping feedback coefficient 2

    juce::Random mRandom;
};
