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

    /** Call before processing begins or when sample rate / channel count changes.
     *  @param operatingSampleRate  The actual rate at which audio will be processed
     *                              (may be upsampled, e.g. 176400 Hz for 4× OS at 44100 Hz).
     *  @param maxBlockSize         Maximum number of samples per block.
     *  @param numChannels          Number of audio channels.
     *  @param originalSampleRate   The host/project sample rate before oversampling.
     *                              If 0 (default), assumed equal to operatingSampleRate
     *                              (i.e. no oversampling). Used only to make
     *                              getLatencyInSamples() return original-rate samples. */
    void prepare(double operatingSampleRate, int maxBlockSize, int numChannels,
                 double originalSampleRate = 0.0);

    /** Process audio in-place.
     *  When sidechainData is non-null, use it for peak detection while applying
     *  gain reduction to channelData. When null, detect on channelData directly. */
    void process(float** channelData, int numChannels, int numSamples,
                 const float* const* sidechainData = nullptr);

    /** Transparent bypass: pass audio through the lookahead delay buffer at unity
     *  gain (no gain reduction applied). This advances the delay buffer so that
     *  the reported latency remains constant whether bypass is on or off. */
    void processBypassDelay(float** channelData, int numChannels, int numSamples);

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

    /** Returns the minimum linear gain applied during the last processed block.
     *  Used by LimiterEngine to compute an accurate combined GR by multiplying
     *  per-stage linear minimums rather than summing per-stage dB values. */
    float getMinGainLinear() const;

    /** Returns the lookahead delay in **original-rate** samples.
     *  When the limiter is prepared at an oversampled rate, this method divides
     *  the internal upsampled-domain delay by the oversampling factor so that
     *  the returned value is always in terms of the host sample rate.
     *  Use LimiterEngine::getLatencySamples() for the final host-facing latency
     *  (which also includes the oversampler FIR latency). */
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

    double mSampleRate         = 44100.0;   // operating rate (may be upsampled)
    double mOriginalSampleRate = 44100.0;   // host rate before oversampling
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

    float mCurrentGRdB = 0.0f;          // reported gain reduction in dB
    float mCurrentMinGainLinear = 1.0f; // minimum linear gain for the last processed block
};
