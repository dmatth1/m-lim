#pragma once

#include "LimiterAlgorithm.h"
#include <vector>

/**
 * TransientLimiter — Stage 1 fast peak limiter with lookahead.
 *
 * Uses a circular lookahead delay buffer to anticipate peaks and apply
 * gain reduction before they arrive at the output. Supports per-channel
 * linking, soft knee, adaptive release, and optional soft saturation.
 *
 * Signal flow per sample:
 *   1. Write input into delay buffer (length = lookahead samples)
 *   2. Scan delay buffer for maximum peak
 *   3. Compute required gain with soft-knee curve
 *   4. Smooth gain: instant attack, exponential release
 *   5. Read delayed output and multiply by smoothed gain
 *   6. Optionally apply soft saturation (tanh waveshaping)
 */
class TransientLimiter
{
public:
    TransientLimiter() = default;

    /** Call before processing begins or when sample rate / channel count changes. */
    void prepare(double sampleRate, int maxBlockSize, int numChannels);

    /** Process audio in-place.
     *  When sidechainData is non-null, use it for peak detection while applying
     *  gain reduction to channelData. When null, detect on channelData directly. */
    void process(float** channelData, int numChannels, int numSamples,
                 const float* const* sidechainData = nullptr);

    /** Set lookahead time in milliseconds (0–5 ms). */
    void setLookahead(float ms);

    /** Set stereo channel linking (0 = fully independent, 1 = fully linked). */
    void setChannelLink(float pct);

    /** Update algorithm parameters (attack shape, release, saturation, knee). */
    void setAlgorithmParams(const AlgorithmParams& params);

    /** Returns current gain reduction in dB (0 = no reduction, negative = reducing). */
    float getGainReduction() const;

private:
    /** Compute the linear gain factor needed to bring peakAbs below threshold,
     *  applying the soft-knee curve from mParams. Returns a value in (0, 1]. */
    float computeRequiredGain(float peakAbs) const;

    /** Soft saturation: tanh waveshaping scaled by amount (0 = bypass, 1 = full). */
    static float softSaturate(float x, float amount);

    double mSampleRate    = 44100.0;
    int    mNumChannels   = 2;
    int    mLookaheadSamples = 0;   // current lookahead size in samples
    int    mMaxLookaheadSamples = 0; // allocated buffer size
    float  mChannelLink   = 1.0f;   // 0–1
    float  mThreshold     = 1.0f;   // linear ceiling (default 0 dBFS)
    AlgorithmParams mParams{};

    // Per-channel circular delay buffers (size = mMaxLookaheadSamples + 1)
    std::vector<std::vector<float>> mDelayBuffers;
    std::vector<int> mWritePos;

    // Per-channel smoothed gain state (linear, 0–1; 1 = unity, <1 = reducing)
    std::vector<float> mGainState;

    float mReleaseCoeff = 0.0f;   // per-sample exponential release coefficient

    float mCurrentGRdB = 0.0f;   // reported gain reduction in dB
};
