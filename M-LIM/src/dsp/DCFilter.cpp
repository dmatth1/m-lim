#include "DCFilter.h"
#include <cmath>

static constexpr double kCutoffHz = 5.0;
static constexpr double kTwoPi = 6.283185307179586;

void DCFilter::prepare(double sampleRate)
{
    R = 1.0 - (kTwoPi * kCutoffHz / sampleRate);
    reset();
}

void DCFilter::process(float* data, int numSamples)
{
    for (int i = 0; i < numSamples; ++i)
    {
        float x = data[i];
        float y = x - xPrev + static_cast<float>(R) * yPrev;
        xPrev = x;
        yPrev = y;
        data[i] = y;
    }
}

void DCFilter::reset()
{
    xPrev = 0.0f;
    yPrev = 0.0f;
}
