#pragma once

#include "LimiterAlgorithm.h"
#include <cstdint>
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
 *   4. Smooth gain: attack speed controlled by transientAttackCoeff (0=slow IIR, 1=instant), exponential release
 *   5. Read delayed output and multiply by smoothed gain
 *   6. Optionally apply soft saturation (tanh waveshaping)
 */
class TransientLimiter
{
public:
    static constexpr int kMaxChannels = 8;

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

    /** Set the limiter threshold in linear scale (e.g. 0.891 for -1 dBFS).
     *  Defaults to 1.0 (0 dBFS). Should be called whenever the output ceiling changes. */
    void setThreshold(float linear);

    /** Set stereo channel linking (0 = fully independent, 1 = fully linked). */
    void setChannelLink(float pct);

    /** Update algorithm parameters (attack shape, release, saturation, knee). */
    void setAlgorithmParams(const AlgorithmParams& params);

    /** Returns current gain reduction in dB (0 = no reduction, negative = reducing). */
    float getGainReduction() const;

    /** Returns the lookahead delay in samples **at the rate this instance was prepared with**.
     *  When the limiter is prepared at an oversampled rate (as done by LimiterEngine),
     *  this value is in upsampled-domain samples, NOT original-rate samples.
     *  Do NOT use this value directly for host latency reporting.
     *  Use LimiterEngine::getLatencySamples() for the host-facing latency. */
    int getLatencyInSamples() const;

    /** Reset the sliding-window deques and write counters to start at the given
     *  offset. Used in tests to pre-seed counters near overflow boundaries. */
    void resetCounters(int64_t startOffset = 0);

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

    // Cached dB/linear knee thresholds — updated by updateKneeCache().
    // Avoids redundant gainToDecibels(mThreshold) calls in the per-sample inner loop.
    float mThresholdDb           = 0.0f;  // gainToDecibels(mThreshold)
    float mLowerKneeDb           = 0.0f;  // mThresholdDb - kneeHalf
    float mUpperKneeDb           = 0.0f;  // mThresholdDb + kneeHalf
    float mLinearLowerKneeThresh = 1.0f;  // decibelsToGain(mLowerKneeDb)

    /** Recompute all cached knee/threshold values. Call after any change to
     *  mThreshold or mParams.kneeWidth. */
    void updateKneeCache();

    // Per-channel circular delay buffers for main audio (size = mMaxLookaheadSamples + 1)
    std::vector<std::vector<float>> mDelayBuffers;
    std::vector<int> mWritePos;

    // Per-channel circular delay buffers for sidechain detection signal
    std::vector<std::vector<float>> mSidechainDelayBuffers;
    std::vector<int> mSidechainWritePos;

    // Per-channel smoothed gain state (linear, 0–1; 1 = unity, <1 = reducing)
    std::vector<float> mGainState;

    // Sliding-window maximum deque for O(1) amortised peak detection.
    // Maintained as a monotone non-increasing deque: the front is always
    // the window maximum.  A fixed-size circular buffer avoids heap
    // allocations on the audio thread.
    struct SWDequeEntry { float value; int64_t pos; };
    struct SWDeque
    {
        std::vector<SWDequeEntry> data; // pre-allocated, capacity = mMaxLookaheadSamples + 1
        int head = 0, tail = 0, count = 0;

        void reserve (int cap)
        {
            data.assign (cap + 1, SWDequeEntry{});
            head = tail = count = 0;
        }
        void reset () noexcept { head = tail = count = 0; }
        bool empty () const noexcept { return count == 0; }
        SWDequeEntry& front () noexcept { return data[head]; }
        SWDequeEntry& back  () noexcept
        {
            int idx = tail - 1;
            if (idx < 0) idx += (int)data.size();
            return data[idx];
        }
        void push_back (SWDequeEntry e) noexcept
        {
            data[tail] = e;
            tail = (tail + 1) % (int)data.size();
            ++count;
        }
        void pop_back () noexcept
        {
            tail = (tail - 1 + (int)data.size()) % (int)data.size();
            --count;
        }
        void pop_front () noexcept
        {
            head = (head + 1) % (int)data.size();
            --count;
        }
    };

    std::vector<SWDeque> mMainDeques;      // one per channel — main-path peak detection
    std::vector<SWDeque> mSCDeques;        // one per channel — sidechain-path peak detection
    std::vector<int64_t> mMainWriteCount;  // monotone write counter per channel (main)
    std::vector<int64_t> mSCWriteCount;   // monotone write counter per channel (sidechain)

    float mReleaseCoeff = 0.0f;   // per-sample exponential release coefficient

    float mCurrentGRdB = 0.0f;   // reported gain reduction in dB
};
