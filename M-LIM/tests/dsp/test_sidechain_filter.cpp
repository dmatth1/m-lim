#include "catch2/catch_amalgamated.hpp"
#include "dsp/SidechainFilter.h"
#include <juce_audio_basics/juce_audio_basics.h>
#include <cmath>
#include <vector>

static constexpr double kSampleRate   = 44100.0;
static constexpr double kTwoPi        = 6.283185307179586;
static constexpr int    kSettleSamples = 8192;  // let filters reach steady state
static constexpr int    kMeasureSamples = 8192; // samples used for RMS measurement

/** Generate a single-channel AudioBuffer filled with a sine wave. */
static juce::AudioBuffer<float> makeSine(double freq, double amplitude,
                                          int numSamples, double sampleRate)
{
    juce::AudioBuffer<float> buf(1, numSamples);
    float* data = buf.getWritePointer(0);
    for (int i = 0; i < numSamples; ++i)
        data[i] = static_cast<float>(amplitude * std::sin(kTwoPi * freq * i / sampleRate));
    return buf;
}

/** Compute RMS of a mono AudioBuffer. */
static double rms(const juce::AudioBuffer<float>& buf)
{
    const float* data = buf.getReadPointer(0);
    double sum = 0.0;
    for (int i = 0; i < buf.getNumSamples(); ++i)
        sum += static_cast<double>(data[i]) * data[i];
    return std::sqrt(sum / buf.getNumSamples());
}

// ---------------------------------------------------------------------------
// test_highpass_attenuates_bass
// ---------------------------------------------------------------------------
TEST_CASE("test_highpass_attenuates_bass", "[SidechainFilter]")
{
    SidechainFilter filter;
    filter.prepare(kSampleRate, kSettleSamples + kMeasureSamples);
    filter.setHighPassFreq(100.0f);  // 100 Hz HP cutoff

    const int totalSamples = kSettleSamples + kMeasureSamples;
    auto buf = makeSine(50.0, 1.0, totalSamples, kSampleRate); // 50 Hz stimulus
    filter.process(buf);

    // Measure RMS in the latter portion (filter has settled)
    juce::AudioBuffer<float> measureBuf(1, kMeasureSamples);
    measureBuf.copyFrom(0, 0, buf, 0, kSettleSamples, kMeasureSamples);

    const double rmsOut = rms(measureBuf);
    const double rmsIn  = 1.0 / std::sqrt(2.0); // RMS of unit sine
    const double attenuationDb = 20.0 * std::log10(rmsOut / rmsIn);

    // A 2nd-order Butterworth HP at 100 Hz attenuates 50 Hz by ~12 dB.
    REQUIRE(attenuationDb < -6.0);
}

// ---------------------------------------------------------------------------
// test_lowpass_attenuates_treble
// ---------------------------------------------------------------------------
TEST_CASE("test_lowpass_attenuates_treble", "[SidechainFilter]")
{
    SidechainFilter filter;
    filter.prepare(kSampleRate, kSettleSamples + kMeasureSamples);
    filter.setLowPassFreq(5000.0f);  // 5 kHz LP cutoff

    const int totalSamples = kSettleSamples + kMeasureSamples;
    auto buf = makeSine(10000.0, 1.0, totalSamples, kSampleRate); // 10 kHz stimulus
    filter.process(buf);

    juce::AudioBuffer<float> measureBuf(1, kMeasureSamples);
    measureBuf.copyFrom(0, 0, buf, 0, kSettleSamples, kMeasureSamples);

    const double rmsOut = rms(measureBuf);
    const double rmsIn  = 1.0 / std::sqrt(2.0);
    const double attenuationDb = 20.0 * std::log10(rmsOut / rmsIn);

    // A 2nd-order Butterworth LP at 5 kHz attenuates 10 kHz by ~12 dB.
    REQUIRE(attenuationDb < -6.0);
}

// ---------------------------------------------------------------------------
// test_tilt_boosts_highs
// ---------------------------------------------------------------------------
TEST_CASE("test_tilt_boosts_highs", "[SidechainFilter]")
{
    // With a positive tilt, high-frequency content should have greater
    // amplitude than low-frequency content for the same input level.

    const int totalSamples = kSettleSamples + kMeasureSamples;

    auto runAtFreq = [&](double freq) -> double
    {
        SidechainFilter f;
        f.prepare(kSampleRate, totalSamples);
        f.setTilt(6.0f); // +6 dB tilt
        auto buf = makeSine(freq, 1.0, totalSamples, kSampleRate);
        f.process(buf);

        juce::AudioBuffer<float> measureBuf(1, kMeasureSamples);
        measureBuf.copyFrom(0, 0, buf, 0, kSettleSamples, kMeasureSamples);
        return rms(measureBuf);
    };

    const double rmsLow  = runAtFreq(100.0);   // well below 1 kHz pivot
    const double rmsHigh = runAtFreq(10000.0); // well above 1 kHz pivot

    // Positive tilt: high frequencies should be louder than low frequencies.
    REQUIRE(rmsHigh > rmsLow);
}

// ---------------------------------------------------------------------------
// test_flat_passes_signal
// ---------------------------------------------------------------------------
TEST_CASE("test_flat_passes_signal", "[SidechainFilter]")
{
    // Default settings (HP=20 Hz, LP=20 kHz, tilt=0) should be transparent
    // for a 1 kHz tone.

    SidechainFilter filter;
    filter.prepare(kSampleRate, kSettleSamples + kMeasureSamples);
    // No parameter changes — use defaults

    const int totalSamples = kSettleSamples + kMeasureSamples;
    auto buf = makeSine(1000.0, 1.0, totalSamples, kSampleRate);
    filter.process(buf);

    juce::AudioBuffer<float> measureBuf(1, kMeasureSamples);
    measureBuf.copyFrom(0, 0, buf, 0, kSettleSamples, kMeasureSamples);

    const double rmsOut = rms(measureBuf);
    const double rmsIn  = 1.0 / std::sqrt(2.0);

    // Attenuation at 1 kHz should be < 0.1 dB
    const double attenuationDb = 20.0 * std::log10(rmsOut / rmsIn);
    REQUIRE(std::abs(attenuationDb) < 0.1);
}
