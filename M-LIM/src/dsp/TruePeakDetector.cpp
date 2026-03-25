#include "TruePeakDetector.h"
#include <cmath>
#include <algorithm>

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
