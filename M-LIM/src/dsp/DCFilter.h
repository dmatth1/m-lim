#pragma once

/**
 * DCFilter — first-order high-pass IIR filter for DC offset removal.
 * Cutoff frequency ~5 Hz; effectively transparent to audio content.
 *
 * Transfer function: y[n] = x[n] - x[n-1] + R * y[n-1]
 * where R = 1 - (2*pi*fc / sampleRate)
 */
class DCFilter
{
public:
    DCFilter() = default;

    /** Call before processing begins or when sample rate changes. */
    void prepare(double sampleRate);

    /** Process samples in-place. */
    void process(float* data, int numSamples);

    /** Clear internal filter state. */
    void reset();

private:
    double R = 0.9996;   // default for 44100 Hz / 5 Hz cutoff
    float xPrev = 0.0f;
    float yPrev = 0.0f;
};
