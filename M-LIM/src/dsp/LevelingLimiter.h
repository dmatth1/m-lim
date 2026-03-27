#pragma once

#include "LimiterAlgorithm.h"
#include <vector>

/**
 * LevelingLimiter — Stage 2 slow release limiter.
 *
 * Provides gentle, program-dependent level control after the TransientLimiter
 * (Stage 1) has caught the fast peaks. Uses an envelope follower with
 * configurable attack and release times, optional adaptive release, and
 * stereo channel linking.
 *
 * Signal flow per sample:
 *   1. Detect peak on each channel (or sidechain input)
 *   2. Compute required gain to stay below 0 dBFS
 *   3. Channel linking: blend per-channel gains toward minimum
 *   4. Smooth gain: exponential attack + shaped exponential release
 *   5. Apply gain to main audio in-place
 */
class LevelingLimiter
{
public:
    LevelingLimiter() = default;

    /** Call before processing begins or when sample rate / channel count changes. */
    void prepare(double sampleRate, int maxBlockSize, int numChannels);

    /** Clear gain/envelope state without reallocating vectors.
     *  Safe to call from the audio thread. */
    void reset();

    /** Process audio in-place.
     *  When sidechainData is non-null, use it for envelope following while
     *  applying gain reduction to channelData. When null, follow channelData. */
    void process(float** channelData, int numChannels, int numSamples,
                 const float* const* sidechainData = nullptr);

    /** Set the limiter threshold in linear scale (e.g. 0.891 for -1 dBFS).
     *  Defaults to 1.0 (0 dBFS). Should be called whenever the output ceiling changes. */
    void setThreshold(float linear);

    /** Set attack time in milliseconds (0–100 ms). */
    void setAttack(float ms);

    /** Set release time in milliseconds (10–1000 ms). */
    void setRelease(float ms);

    /** Set stereo channel linking (0 = fully independent, 1 = fully linked). */
    void setChannelLink(float pct);

    /** Update algorithm parameters (release shape, adaptive release). */
    void setAlgorithmParams(const AlgorithmParams& params);

    /** Returns current gain reduction in dB (0 = no reduction, negative = reducing). */
    float getGainReduction() const;

private:
    /** Compute the linear gain needed to bring peakAbs at or below threshold. */
    float computeRequiredGain(float peakAbs) const;

    /** Recalculate time-varying coefficients from current ms settings. */
    void updateCoefficients();

    double mSampleRate  = 44100.0;
    int    mNumChannels = 2;

    float mChannelLink  = 1.0f;    // 0–1
    float mThreshold    = 1.0f;    // linear ceiling (default 0 dBFS)

    float mAttackMs     = 10.0f;
    float mReleaseMs    = 100.0f;
    float mAttackCoeff  = 0.0f;    // per-sample exponential attack coefficient
    float mReleaseCoeff = 0.0f;    // per-sample exponential release coefficient

    // Slow smoother for adaptive-release detection (~500 ms time constant)
    float mAdaptiveSmoothCoeff = 0.0f;

    AlgorithmParams mParams{};

    // Per-channel state
    std::vector<float> mGainState;  // smoothed gain in linear scale (1.0 = no reduction)
    std::vector<float> mEnvState;   // long-term smoothed gain in linear scale for adaptive release (1.0 = no reduction)

    float mCurrentGRdB = 0.0f;  // reported gain reduction in dB
};
