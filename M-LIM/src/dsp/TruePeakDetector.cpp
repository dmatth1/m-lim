#include "TruePeakDetector.h"
#include <cmath>
#include <algorithm>

// ITU-R BS.1770-4 Table 1 polyphase FIR coefficients — 4 phases x 12 taps
// Phase 0 and Phase 3 are mirrors; Phase 1 and Phase 2 are mirrors.
const float TruePeakDetector::kCoeffs[TruePeakDetector::kPhases][TruePeakDetector::kFirTaps] = {
    // Phase 0 (near-identity, main lobe at tap 6)
    {  0.0017089843750f,  0.0109863281250f, -0.0196533203125f,  0.0332031250000f,
      -0.0594482421875f,  0.1373291015625f,  0.9721679687500f, -0.1022949218750f,
       0.0476074218750f, -0.0266113281250f,  0.0148925781250f, -0.0083007812500f },
    // Phase 1 (1/4 sample offset)
    { -0.0291748046875f,  0.0292968750000f, -0.0517578125000f,  0.0891113281250f,
      -0.1665039062500f,  0.4650878906250f,  0.7797851562500f, -0.2003173828125f,
       0.1015625000000f, -0.0582275390625f,  0.0330810546875f, -0.0189208984375f },
    // Phase 2 (2/4 sample offset, mirror of Phase 1)
    { -0.0189208984375f,  0.0330810546875f, -0.0582275390625f,  0.1015625000000f,
      -0.2003173828125f,  0.7797851562500f,  0.4650878906250f, -0.1665039062500f,
       0.0891113281250f, -0.0517578125000f,  0.0292968750000f, -0.0291748046875f },
    // Phase 3 (3/4 sample offset, mirror of Phase 0)
    { -0.0083007812500f,  0.0148925781250f, -0.0266113281250f,  0.0476074218750f,
      -0.1022949218750f,  0.9721679687500f,  0.1373291015625f, -0.0594482421875f,
       0.0332031250000f, -0.0196533203125f,  0.0109863281250f,  0.0017089843750f }
};

void TruePeakDetector::prepare(double sampleRate)
{
    mSampleRate = sampleRate;
    reset();
}

float TruePeakDetector::processSample(float sample)
{
    // Write sample into circular buffer
    mBuffer[mWritePos] = sample;
    mWritePos = (mWritePos + 1) % kFirTaps;

    // Compute all 4 phase interpolations and take the max absolute value
    float maxAbs = 0.0f;
    for (int phase = 0; phase < kPhases; ++phase)
    {
        float sum = 0.0f;
        for (int tap = 0; tap < kFirTaps; ++tap)
        {
            // Read from circular buffer: newest sample is at (mWritePos - 1)
            int idx = (mWritePos - 1 - tap + kFirTaps) % kFirTaps;
            sum += kCoeffs[phase][tap] * mBuffer[idx];
        }
        float absVal = std::abs(sum);
        if (absVal > maxAbs)
            maxAbs = absVal;
    }

    if (maxAbs > mPeak)
        mPeak = maxAbs;

    return maxAbs;
}

void TruePeakDetector::processBlock(const float* input, int numSamples)
{
    for (int i = 0; i < numSamples; ++i)
        processSample(input[i]);
}

float TruePeakDetector::getPeak() const
{
    return mPeak;
}

void TruePeakDetector::reset()
{
    mBuffer.fill(0.0f);
    mWritePos = 0;
    mPeak = 0.0f;
}
