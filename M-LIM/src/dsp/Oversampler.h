#pragma once

#include <juce_dsp/juce_dsp.h>
#include <atomic>

/**
 * Oversampler — wraps juce::dsp::Oversampling<float> for the limiter DSP chain.
 *
 * Factor mapping: 0=off (1x), 1=2x, 2=4x, 3=8x, 4=16x, 5=32x
 *
 * Usage:
 *   prepare(sampleRate, maxBlockSize, numChannels);
 *   auto upBlock = upsample(buffer);   // returns block at N*factor samples
 *   // ... process upBlock ...
 *   downsample(buffer);                // writes back to original buffer
 */
class Oversampler
{
public:
    Oversampler() = default;
    ~Oversampler() = default;

    /** Call before processing begins or when sample rate / block size changes. */
    void prepare(double sampleRate, int maxBlockSize, int numChannels);

    /** Upsample the buffer. Returns an AudioBlock at the higher sample rate.
     *  When factor == 0, returns a block wrapping the original buffer (passthrough). */
    juce::dsp::AudioBlock<float> upsample(juce::AudioBuffer<float>& buffer);

    /** Downsample the previously upsampled signal back into buffer.
     *  When factor == 0, this is a no-op. */
    void downsample(juce::AudioBuffer<float>& buffer);

    /** Set oversampling factor. 0=off, 1=2x, 2=4x, 3=8x, 4=16x, 5=32x.
     *  Re-creates the internal oversampling object when the factor changes.
     *  WARNING: allocates memory — do not call from the audio thread. */
    void setFactor(int factor);

    int getFactor() const;

    /** Request a factor change without immediate rebuild (real-time safe).
     *  Call needsRebuild() to check if a rebuild is pending, then
     *  commitRebuild() from a non-real-time thread to apply the change. */
    void requestFactor(int pendingFactor);

    /** Returns true if a factor change was requested but not yet committed. */
    bool needsRebuild() const;

    /** Apply the pending factor and rebuild the oversampling object.
     *  Allocates memory — must be called from a non-real-time thread. */
    void commitRebuild();

    /** Returns the latency introduced by oversampling in samples (at the original rate).
     *  Returns 0 when factor == 0. */
    float getLatencySamples() const;

private:
    void recreate();

    std::unique_ptr<juce::dsp::Oversampling<float>> mOversampling;
    double             mSampleRate   = 44100.0;
    int                mMaxBlockSize = 512;
    int                mNumChannels  = 2;
    int                mFactor       = 0;
    bool               mPrepared     = false;
    std::atomic<int>   mPendingFactor { 0 };
};
