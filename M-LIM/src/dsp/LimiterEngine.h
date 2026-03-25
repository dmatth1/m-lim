#pragma once

#include "MeterData.h"
#include "LimiterAlgorithm.h"
#include "TransientLimiter.h"
#include "LevelingLimiter.h"
#include "Oversampler.h"
#include "TruePeakDetector.h"
#include "SidechainFilter.h"
#include "DCFilter.h"
#include "Dither.h"

#include <juce_audio_basics/juce_audio_basics.h>
#include <atomic>
#include <vector>

/**
 * LimiterEngine — top-level DSP orchestrator for M-LIM.
 *
 * DSP chain (per process() call):
 *   1. Apply input gain
 *   2. Make sidechain copy and run SidechainFilter on it
 *   3. Oversample up (both main and sidechain if enabled)
 *   4. TransientLimiter (Stage 1) with optional sidechain detection
 *   5. LevelingLimiter (Stage 2)
 *   6. Oversample down
 *   7. Apply output ceiling (hard clip)
 *   8. DCFilter (optional)
 *   9. Dither (optional)
 *  10. Meter and push MeterData to FIFO
 *
 * Bypass mode: pass audio through unchanged, still meter.
 * Delta mode:  output = (input after gain) - (limited output), i.e., "what was removed".
 * Unity gain:  ceiling = -inputGain so net output is at 0 dBFS for a 0 dBFS input.
 */
class LimiterEngine
{
public:
    LimiterEngine();
    ~LimiterEngine() = default;

    // Non-copyable
    LimiterEngine(const LimiterEngine&) = delete;
    LimiterEngine& operator=(const LimiterEngine&) = delete;

    // -----------------------------------------------------------------------
    // Setup — call from message/UI thread before processing begins
    // -----------------------------------------------------------------------

    void prepare(double sampleRate, int maxBlockSize, int numChannels);

    // -----------------------------------------------------------------------
    // Processing — call from audio thread
    // -----------------------------------------------------------------------

    void process(juce::AudioBuffer<float>& buffer);

    // -----------------------------------------------------------------------
    // Parameter setters — thread-safe via atomics (call from any thread)
    // -----------------------------------------------------------------------

    void setAlgorithm(LimiterAlgorithm algo);
    void setInputGain(float dB);
    void setOutputCeiling(float dB);
    void setLookahead(float ms);
    void setAttack(float ms);
    void setRelease(float ms);
    void setChannelLinkTransients(float pct);
    void setChannelLinkRelease(float pct);
    void setOversamplingFactor(int factor);   // 0=off, 1=2x … 5=32x
    void setTruePeakEnabled(bool enabled);
    void setDCFilterEnabled(bool enabled);
    void setDitherEnabled(bool enabled);
    void setDitherBitDepth(int bits);
    void setDitherNoiseShaping(int mode);     // 0=none, 1=first-order, 2=second-order
    void setBypass(bool bypass);
    void setDeltaMode(bool delta);
    void setUnityGain(bool unity);
    void setSidechainHPFreq(float hz);   ///< Detection-path high-pass: 20-2000 Hz
    void setSidechainLPFreq(float hz);   ///< Detection-path low-pass: 2000-20000 Hz
    void setSidechainTilt(float dB);     ///< Detection-path tilt EQ: -6 to +6 dB

    // -----------------------------------------------------------------------
    // State queries — thread-safe reads (call from any thread)
    // -----------------------------------------------------------------------

    float getGainReduction() const;   ///< Current GR in dB (0 = none, negative = reducing)
    float getTruePeakL() const;
    float getTruePeakR() const;
    int   getLatencySamples() const;  ///< Total latency for host compensation

    /** Returns true if an oversampling factor change was detected on the audio thread
     *  but the actual reallocation is pending (must be done off the audio thread).
     *  Call applyDeferredOversamplingChange() from a non-RT thread to complete it. */
    bool hasDeferredOversamplingChange() const { return mDeferredOversamplingChange.load(); }

    // -----------------------------------------------------------------------
    // Metering FIFO — push from audio thread, pop from UI thread
    // -----------------------------------------------------------------------

    LockFreeFIFO<MeterData>& getMeterFIFO() { return mMeterFIFO; }

private:
    // -----------------------------------------------------------------------
    // DSP components
    // -----------------------------------------------------------------------
    Oversampler      mOversampler;
    Oversampler      mSidechainOversampler;  // mirrors mOversampler for sidechain path
    TransientLimiter mTransientLimiter;
    LevelingLimiter  mLevelingLimiter;
    TruePeakDetector mTruePeakL;
    TruePeakDetector mTruePeakR;
    SidechainFilter  mSidechainFilter;
    DCFilter         mDCFilterL;
    DCFilter         mDCFilterR;
    Dither           mDitherL;
    Dither           mDitherR;

    // -----------------------------------------------------------------------
    // Configuration (set from any thread, read on audio thread)
    // -----------------------------------------------------------------------
    std::atomic<float> mInputGainLinear { 1.0f };
    std::atomic<float> mOutputCeilingLinear { 1.0f };
    std::atomic<int>   mAlgorithm { static_cast<int>(LimiterAlgorithm::Transparent) };
    std::atomic<bool>  mTruePeakEnabled { true };
    std::atomic<bool>  mDCFilterEnabled { false };
    std::atomic<bool>  mDitherEnabled   { false };
    std::atomic<bool>  mBypass          { false };
    std::atomic<bool>  mDeltaMode       { false };
    std::atomic<bool>  mUnityGain       { false };

    // Parameters that require re-preparing DSP components — set on audio thread via flags
    std::atomic<bool>  mParamsDirty { false };
    // deferredOversamplingChange — set on audio thread when oversampling factor differs
    // from current. Signals the PluginProcessor to schedule a rebuild via AsyncUpdater
    // so no memory allocation occurs on the audio thread.
    std::atomic<bool>  mDeferredOversamplingChange { false };
    std::atomic<float> mLookaheadMs { 1.0f };
    std::atomic<float> mAttackMs    { 10.0f };
    std::atomic<float> mReleaseMs   { 100.0f };
    std::atomic<float> mChannelLinkTransients { 1.0f };
    std::atomic<float> mChannelLinkRelease    { 1.0f };
    std::atomic<int>   mOversamplingFactor    { 0 };
    std::atomic<int>   mDitherBitDepth        { 24 };
    std::atomic<int>   mDitherNoiseShaping    { 0 };

    // -----------------------------------------------------------------------
    // State (audio thread only)
    // -----------------------------------------------------------------------
    double mSampleRate   = 44100.0;
    int    mMaxBlockSize = 512;
    int    mNumChannels  = 2;

    // Current oversampling factor on audio thread (latency may differ from mOversamplingFactor)
    int mCurrentOversamplingFactor = 0;

    // Metering
    std::atomic<float> mGRdB    { 0.0f };
    std::atomic<float> mTruePkL { 0.0f };
    std::atomic<float> mTruePkR { 0.0f };

    LockFreeFIFO<MeterData> mMeterFIFO { 32 };

    // Working buffers (allocated in prepare, used on audio thread)
    std::vector<std::vector<float>> mSidechainData; // per-channel sidechain copy

    // Pre-allocated buffers and pointer arrays — no heap allocs inside process()
    juce::AudioBuffer<float>  mPreLimitBuffer;   // for delta mode snapshot
    juce::AudioBuffer<float>  mSidechainBuffer;  // sidechain copy (replaces local sideBuf)
    std::vector<float*>        mUpPtrs;           // upsampled channel write pointers
    std::vector<const float*>  mSidePtrs;         // sidechain channel read pointers

    // -----------------------------------------------------------------------
    // Helpers
    // -----------------------------------------------------------------------
    void applyPendingParams();
    void buildMeterData(const juce::AudioBuffer<float>& inputBefore,
                        const juce::AudioBuffer<float>& output);
};
