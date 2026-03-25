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

    // ITU-R BS.1770-4 Table 1 polyphase FIR coefficients
    static constexpr float kCoeffs[kPhases][kFirTaps] = {
        // Phase 0
        {  0.0017089843750f,  0.0109863281250f, -0.0196533203125f,  0.0332031250000f,
          -0.0594482421875f,  0.1373291015625f,  0.9721679687500f, -0.1022949218750f,
           0.0476074218750f, -0.0266113281250f,  0.0148925781250f, -0.0083007812500f },
        // Phase 1
        { -0.0291748046875f,  0.0292968750000f, -0.0517578125000f,  0.0891113281250f,
          -0.1665039062500f,  0.4650878906250f,  0.7797851562500f, -0.2003173828125f,
           0.1015625000000f, -0.0582275390625f,  0.0330810546875f, -0.0189208984375f },
        // Phase 2
        { -0.0189208984375f,  0.0330810546875f, -0.0582275390625f,  0.1015625000000f,
          -0.2003173828125f,  0.7797851562500f,  0.4650878906250f, -0.1665039062500f,
           0.0891113281250f, -0.0517578125000f,  0.0292968750000f, -0.0291748046875f },
        // Phase 3
        { -0.0083007812500f,  0.0148925781250f, -0.0266113281250f,  0.0476074218750f,
          -0.1022949218750f,  0.9721679687500f,  0.1373291015625f, -0.0594482421875f,
           0.0332031250000f, -0.0196533203125f,  0.0109863281250f,  0.0017089843750f }
    };

    // Circular buffer for FIR history (12 samples)
    std::array<float, kFirTaps> mBuffer{};
    int mWritePos = 0;
    float mPeak = 0.0f;
    double mSampleRate = 44100.0;
};
