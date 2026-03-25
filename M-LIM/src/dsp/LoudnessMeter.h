#pragma once

#include <juce_audio_basics/juce_audio_basics.h>
#include <vector>
#include <deque>
#include <limits>

/**
 * LoudnessMeter — ITU-R BS.1770-4 compliant loudness metering.
 *
 * Measures momentary (400 ms), short-term (3 s), and integrated LUFS,
 * plus loudness range (LRA) as defined in EBU R128 / BS.1770-4.
 *
 * K-weighting is applied in two stages:
 *   Stage 1: pre-filter (high-shelf ~+4 dB above ~1.5 kHz)
 *   Stage 2: RLB weighting (2nd-order high-pass at ~38 Hz)
 *
 * Internal processing uses 100 ms analysis blocks. Momentary uses the last
 * 4 blocks (400 ms), short-term uses the last 30 blocks (3 s).
 */
class LoudnessMeter
{
public:
    LoudnessMeter() = default;

    /** Prepare the meter for a given sample rate and channel count.
     *  Must be called before processBlock(). */
    void prepare(double sampleRate, int numChannels);

    /** Process one block of audio. Call from the audio thread. */
    void processBlock(const juce::AudioBuffer<float>& buffer);

    /** Momentary loudness (400 ms window), in LUFS.
     *  Returns -infinity when the window is silent. */
    float getMomentaryLUFS() const;

    /** Short-term loudness (3 s window), in LUFS.
     *  Returns -infinity when the window is silent. */
    float getShortTermLUFS() const;

    /** Integrated loudness since last resetIntegrated() call, in LUFS.
     *  Returns -infinity before enough data has been accumulated. */
    float getIntegratedLUFS() const;

    /** Loudness range (EBU R128), in LU. */
    float getLoudnessRange() const;

    /** Reset the integrated loudness and LRA accumulation. */
    void resetIntegrated();

private:
    // -----------------------------------------------------------------------
    // Internal biquad filter (direct form II transposed)
    // -----------------------------------------------------------------------
    struct Biquad
    {
        double b0 = 1.0, b1 = 0.0, b2 = 0.0;
        double a1 = 0.0, a2 = 0.0;
        double z1 = 0.0, z2 = 0.0;

        float process(float x) noexcept
        {
            double xd = static_cast<double>(x);
            double y  = b0 * xd + z1;
            z1 = b1 * xd - a1 * y + z2;
            z2 = b2 * xd - a2 * y;
            return static_cast<float>(y);
        }

        void reset() noexcept { z1 = z2 = 0.0; }
    };

    // K-weighting filter pair per channel
    std::vector<Biquad> preFilters;   // Stage 1: high-shelf pre-filter
    std::vector<Biquad> rlbFilters;   // Stage 2: RLB high-pass

    double mSampleRate = 44100.0;
    int    mNumChannels = 2;

    // -----------------------------------------------------------------------
    // 100 ms block accumulation
    // -----------------------------------------------------------------------
    int    mBlockSize   = 0;   // samples per 100 ms block at current sample rate
    int    mBlockAccum  = 0;   // sample counter within current block
    double mBlockPower  = 0.0; // accumulated sum-of-squares for current block (all channels)

    // Ring buffers of block mean-square values
    // Momentary = last 4 × 100 ms = 400 ms
    // Short-term = last 30 × 100 ms = 3 s
    static constexpr int kMomentaryBlocks  = 4;
    static constexpr int kShortTermBlocks  = 30;

    std::deque<double> mMomentaryBuffer;  // up to 4 blocks
    std::deque<double> mShortTermBuffer;  // up to 30 blocks

    // -----------------------------------------------------------------------
    // Integrated loudness gating (BS.1770-4)
    // Gating uses 400 ms windows with 75 % overlap (100 ms hop).
    // We store mean-square values for each 100 ms block and combine 4 at a time.
    // -----------------------------------------------------------------------
    std::vector<double> mGatedBlockHistory;  // all 100 ms block powers since reset

    // -----------------------------------------------------------------------
    // Results (updated every 100 ms block)
    // -----------------------------------------------------------------------
    float mMomentaryLUFS  = -std::numeric_limits<float>::infinity();
    float mShortTermLUFS  = -std::numeric_limits<float>::infinity();
    float mIntegratedLUFS = -std::numeric_limits<float>::infinity();
    float mLoudnessRange  = 0.0f;

    // -----------------------------------------------------------------------
    // Helpers
    // -----------------------------------------------------------------------
    void setupKWeightingFilters();
    void onBlockComplete(double blockMeanSquare);
    void updateIntegratedAndLRA();

    /** Convert mean-square power (linear) to LUFS.
     *  LUFS = -0.691 + 10 * log10(power). */
    static float powerToLUFS(double power) noexcept;

    /** LUFS to linear power. */
    static double lufsToLinear(float lufs) noexcept;
};
