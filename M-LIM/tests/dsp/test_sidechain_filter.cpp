#include "catch2/catch_amalgamated.hpp"
#include "dsp/SidechainFilter.h"
#include <juce_audio_basics/juce_audio_basics.h>
#include <atomic>
#include <cmath>
#include <thread>
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

// ---------------------------------------------------------------------------
// test_parameter_change_during_process
// Calls setHighPassFreq() from a second thread while process() runs on the
// main thread. Verifies no crash and that the parameter change eventually
// takes effect (filter reflects the new HP cutoff).
// ---------------------------------------------------------------------------
TEST_CASE("test_parameter_change_during_process", "[SidechainFilter]")
{
    static constexpr int kBlockSize = 256;
    static constexpr int kNumBlocks = 200;

    SidechainFilter filter;
    filter.prepare(kSampleRate, kBlockSize);

    std::atomic<bool> stopWriter  { false };
    std::atomic<bool> writerReady { false };
    std::atomic<int>  writeCount  { 0 };

    // Writer thread: hammers setHighPassFreq() and setLowPassFreq() at a high
    // rate while the audio thread is calling process().
    std::thread writer([&]()
    {
        float hp = 20.0f;
        float lp = 20000.0f;
        writerReady.store(true, std::memory_order_release);
        while (!stopWriter.load(std::memory_order_relaxed))
        {
            filter.setHighPassFreq(hp);
            filter.setLowPassFreq(lp);
            filter.setTilt(0.0f);
            hp = (hp >= 500.0f) ? 20.0f : hp + 10.0f;
            lp = (lp <= 5000.0f) ? 20000.0f : lp - 100.0f;
            writeCount.fetch_add(1, std::memory_order_relaxed);
        }
    });

    // Wait for the writer thread to start before running the audio loop.
    while (!writerReady.load(std::memory_order_acquire)) {}

    // Audio thread: calls process() kNumBlocks times.
    juce::AudioBuffer<float> buf(1, kBlockSize);
    for (int block = 0; block < kNumBlocks; ++block)
    {
        // Fill with a 1 kHz sine
        float* data = buf.getWritePointer(0);
        for (int i = 0; i < kBlockSize; ++i)
            data[i] = static_cast<float>(std::sin(kTwoPi * 1000.0 * i / kSampleRate));

        filter.process(buf); // must not crash
    }

    stopWriter.store(true, std::memory_order_relaxed);
    writer.join();

    // Sanity: writer must have run at least once.
    REQUIRE(writeCount.load() > 0);

    // Now verify the parameter change actually took effect: set a 5 kHz HP
    // and confirm a 100 Hz tone is strongly attenuated after the next process().
    filter.setHighPassFreq(5000.0f);

    const int settleSamples  = 8192;
    const int measureSamples = 4096;
    juce::AudioBuffer<float> testBuf(1, settleSamples + measureSamples);
    float* data = testBuf.getWritePointer(0);
    for (int i = 0; i < settleSamples + measureSamples; ++i)
        data[i] = static_cast<float>(std::sin(kTwoPi * 100.0 * i / kSampleRate));

    filter.process(testBuf);

    // Measure RMS in the settled portion
    double sum = 0.0;
    for (int i = settleSamples; i < settleSamples + measureSamples; ++i)
        sum += static_cast<double>(data[i]) * data[i];
    const double rmsOut      = std::sqrt(sum / measureSamples);
    const double rmsIn       = 1.0 / std::sqrt(2.0);
    const double attenuationDb = 20.0 * std::log10(rmsOut / rmsIn + 1e-12);

    // 5 kHz HP attenuates 100 Hz by at least 20 dB (well below cutoff).
    REQUIRE(attenuationDb < -20.0);
}
