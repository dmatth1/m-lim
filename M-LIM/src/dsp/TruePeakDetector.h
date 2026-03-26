#pragma once
#include <array>
#include <atomic>

/**
 * TruePeakDetector — ITU-R BS.1770-4 compliant inter-sample peak detection.
 *
 * Uses 4x oversampled FIR interpolation with the exact polyphase coefficients
 * from ITU-R BS.1770-4 Table 1 (48-tap total, 4 phases x 12 taps).
 *
 * For each input sample, 4 interpolated values are computed and the maximum
 * absolute value across all phases is used to update the running peak.
 *
 * The hot path uses SIMD (juce::dsp::SIMDRegister<float>) to compute all 4
 * phase FIR outputs in parallel using the tap-major kCoeffsByTap table, with
 * a linear staging buffer to eliminate modulo arithmetic in the inner loop.
 * Falls back to scalar on platforms without 4-wide SIMD float support.
 */
class TruePeakDetector
{
public:
    TruePeakDetector() = default;

    /** Call before processing begins or when sample rate changes. */
    void prepare(double sampleRate);

    /**
     * Process a single sample and return the true peak value for that sample.
     * Also updates the running peak returned by getPeak().
     */
    float processSample(float sample);

    /** Process an entire buffer and update the running peak. */
    void processBlock(const float* input, int numSamples);

    /** Returns the current running true peak level (max seen since last reset). */
    float getPeak() const;

    /** Clear internal state and running peak. */
    void reset();

    /** Reset only the running peak accumulator, preserving FIR filter state.
     *  Use this when starting a new measurement window (e.g., per-block enforcement)
     *  to avoid a cold-FIR warm-up gap at block boundaries.
     *  Use reset() only when completely reinitializing (e.g., after prepare()). */
    void resetPeak();

    /**
     * Scalar FIR path — exposed for testing SIMD vs scalar parity.
     * Computes the same result as processSample() but without SIMD.
     */
    float processSampleScalar(float sample);

private:
    static constexpr int kFirTaps = 12;
    static constexpr int kPhases = 4;

    // ITU-R BS.1770-4 Table 1 polyphase FIR coefficients — phase-major layout
    // kCoeffs[phase][tap] — original layout kept for reference / scalar path
    static const float kCoeffs[kPhases][kFirTaps];

    // Tap-major transposed coefficient table for SIMD: kCoeffsByTap[tap][phase]
    // For each tap, load all 4 phase coefficients at once into a SIMD register.
    static const float kCoeffsByTap[kFirTaps][kPhases];

    // Circular buffer for scalar FIR history (12 samples)
    std::array<float, kFirTaps> mBuffer{};
    int mWritePos = 0;

    // Linear staging buffer for SIMD FIR: size 2*kFirTaps avoids modulo in inner loop.
    // Each new sample is written at mLinearPos and mLinearPos+kFirTaps.
    std::array<float, kFirTaps * 2> mLinearBuf{};
    int mLinearPos = 0;

    std::atomic<float> mPeak { 0.0f };
    double mSampleRate = 44100.0;
};
