#pragma once

#include <juce_dsp/juce_dsp.h>

/**
 * SidechainFilter — shapes the detection signal for the limiter.
 *
 * Applies second-order Butterworth high-pass and low-pass filters plus a
 * first-order tilt EQ (low shelf + high shelf pivoting at 1 kHz) to the
 * audio buffer used by the sidechain detection path.
 *
 * Default settings are effectively flat:
 *   HP = 20 Hz   (far below audible content)
 *   LP = 20000 Hz (at or beyond audible ceiling)
 *   Tilt = 0 dB  (no spectral tilt)
 */
class SidechainFilter
{
public:
    SidechainFilter();

    /** Call before processing begins or when sample rate changes. */
    void prepare(double sampleRate, int maxBlockSize);

    /** Process buffer in-place (operates on all channels, up to 2). */
    void process(juce::AudioBuffer<float>& buffer);

    /** High-pass cutoff frequency in Hz. Range: 20–2000 Hz. */
    void setHighPassFreq(float hz);

    /** Low-pass cutoff frequency in Hz. Range: 2000–20000 Hz. */
    void setLowPassFreq(float hz);

    /** Spectral tilt in dB. Range: -6 to +6 dB.
     *  Positive values boost high frequencies relative to low frequencies,
     *  pivoting around 1 kHz. */
    void setTilt(float dB);

private:
    void updateCoefficients();

    double mSampleRate = 44100.0;
    float  mHPFreq     = 20.0f;
    float  mLPFreq     = 20000.0f;
    float  mTiltDb     = 0.0f;

    static constexpr int kMaxChannels = 2;

    // Per-channel IIR filters
    juce::dsp::IIR::Filter<float> mHP[kMaxChannels];
    juce::dsp::IIR::Filter<float> mLP[kMaxChannels];
    juce::dsp::IIR::Filter<float> mTiltLS[kMaxChannels]; // low shelf
    juce::dsp::IIR::Filter<float> mTiltHS[kMaxChannels]; // high shelf
};
