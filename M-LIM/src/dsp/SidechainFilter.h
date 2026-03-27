#pragma once

#include <juce_dsp/juce_dsp.h>
#include <atomic>

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
 *
 * Thread safety: setHighPassFreq(), setLowPassFreq(), and setTilt() are safe
 * to call from any thread (including the UI/message thread) while process()
 * is running on the audio thread. Parameter changes are stored in atomics and
 * applied at the start of the next process() call on the audio thread.
 */
class SidechainFilter
{
public:
    SidechainFilter();

    /** Call before processing begins or when sample rate changes. */
    void prepare(double sampleRate, int maxBlockSize);

    /** Clear filter state without reallocating coefficients.
     *  Safe to call from the audio thread. */
    void reset();

    /** Process buffer in-place (operates on all channels, up to 2).
     *  Applies any pending parameter changes before processing. */
    void process(juce::AudioBuffer<float>& buffer);

    /** High-pass cutoff frequency in Hz. Range: 20–2000 Hz.
     *  Thread-safe: may be called from any thread. */
    void setHighPassFreq(float hz);

    /** Low-pass cutoff frequency in Hz. Range: 2000–20000 Hz.
     *  Thread-safe: may be called from any thread. */
    void setLowPassFreq(float hz);

    /** Spectral tilt in dB. Range: -6 to +6 dB.
     *  Positive values boost high frequencies relative to low frequencies,
     *  pivoting around 1 kHz.
     *  Thread-safe: may be called from any thread. */
    void setTilt(float dB);

private:
    /** Recalculates filter coefficients from current mHPFreq/mLPFreq/mTiltDb.
     *  Must be called only from the audio thread (inside process()). */
    void updateCoefficients();

    double mSampleRate = 44100.0;

    // These are only read/written on the audio thread (inside process()).
    float  mHPFreq  = 20.0f;
    float  mLPFreq  = 20000.0f;
    float  mTiltDb  = 0.0f;

    // Pending values written by setters (any thread), consumed by process().
    std::atomic<float> mPendingHP   { 20.0f };
    std::atomic<float> mPendingLP   { 20000.0f };
    std::atomic<float> mPendingTilt { 0.0f };
    std::atomic<bool>  mCoeffsDirty { false };

    static constexpr int kMaxChannels = 2;

    // Per-channel IIR filters (accessed only on the audio thread)
    juce::dsp::IIR::Filter<float> mHP[kMaxChannels];
    juce::dsp::IIR::Filter<float> mLP[kMaxChannels];
    juce::dsp::IIR::Filter<float> mTiltLS[kMaxChannels]; // low shelf
    juce::dsp::IIR::Filter<float> mTiltHS[kMaxChannels]; // high shelf

    // Pre-allocated coefficient objects (one per filter slot per channel).
    // Allocated once in prepare() and reused every updateCoefficients() call
    // so no heap allocation occurs on the audio thread.
    using CoeffPtr = juce::dsp::IIR::Coefficients<float>::Ptr;
    CoeffPtr mHPCoeffs[kMaxChannels];
    CoeffPtr mLPCoeffs[kMaxChannels];
    CoeffPtr mLSCoeffs[kMaxChannels];
    CoeffPtr mHSCoeffs[kMaxChannels];
};
