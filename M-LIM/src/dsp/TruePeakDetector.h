#pragma once
#include <array>

/**
 * TruePeakDetector — ITU-R BS.1770-4 compliant inter-sample peak detection.
 *
 * Uses 4x oversampled FIR interpolation with the exact polyphase coefficients
 * from ITU-R BS.1770-4 Table 1 (48-tap total, 4 phases x 12 taps).
 *
 * For each input sample, 4 interpolated values are computed and the maximum
 * absolute value across all phases is used to update the running peak.
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

private:
    static constexpr int kFirTaps = 12;
    static constexpr int kPhases = 4;

    // ITU-R BS.1770-4 Table 1 polyphase FIR coefficients (defined in .cpp)
    static const float kCoeffs[kPhases][kFirTaps];

    // Circular buffer for FIR history (12 samples)
    std::array<float, kFirTaps> mBuffer{};
    int mWritePos = 0;
    float mPeak = 0.0f;
    double mSampleRate = 44100.0;
};
