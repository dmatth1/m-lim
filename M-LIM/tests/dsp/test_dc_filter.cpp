#include "catch2/catch_amalgamated.hpp"
#include "dsp/DCFilter.h"
#include <cmath>
#include <vector>
#include <numeric>
#include <cfloat>

static constexpr double kSampleRate = 44100.0;
static constexpr double kTwoPi = 6.283185307179586;

TEST_CASE("test_removes_dc_offset", "[DCFilter]")
{
    DCFilter filter;
    filter.prepare(kSampleRate);

    const int numSamples = 44100; // 1 second
    std::vector<float> signal(numSamples);

    // DC offset of 0.5 plus a small 100 Hz sine
    for (int i = 0; i < numSamples; ++i)
        signal[i] = 0.5f + 0.1f * static_cast<float>(std::sin(kTwoPi * 100.0 * i / kSampleRate));

    filter.process(signal.data(), numSamples);

    // Measure DC in the latter half (filter settled)
    double sum = 0.0;
    const int start = numSamples / 2;
    for (int i = start; i < numSamples; ++i)
        sum += signal[i];
    double dc = sum / (numSamples - start);

    REQUIRE(std::abs(dc) < 1e-3); // DC should be near zero
}

TEST_CASE("test_passes_audio_signal", "[DCFilter]")
{
    DCFilter filter;
    filter.prepare(kSampleRate);

    const int numSamples = 44100;
    const double freq = 1000.0; // 1 kHz
    std::vector<float> signal(numSamples);
    for (int i = 0; i < numSamples; ++i)
        signal[i] = std::sin(kTwoPi * freq * i / kSampleRate);

    filter.process(signal.data(), numSamples);

    // Measure RMS of latter half (steady state)
    double rmsIn = std::sqrt(0.5); // RMS of unit sine = 1/sqrt(2)
    double sumSq = 0.0;
    const int start = numSamples / 2;
    for (int i = start; i < numSamples; ++i)
        sumSq += signal[i] * signal[i];
    double rmsOut = std::sqrt(sumSq / (numSamples - start));

    // Attenuation at 1 kHz should be < 0.1 dB
    double attenuationDb = 20.0 * std::log10(rmsOut / rmsIn);
    REQUIRE(std::abs(attenuationDb) < 0.1);
}

TEST_CASE("test_reset_clears_state", "[DCFilter]")
{
    DCFilter filter;
    filter.prepare(kSampleRate);

    // Feed a DC signal to prime the filter state
    std::vector<float> data(1000, 1.0f);
    filter.process(data.data(), static_cast<int>(data.size()));

    // Reset and process a single zero sample
    filter.reset();
    float zero = 0.0f;
    filter.process(&zero, 1);

    // After reset, x[n-1] and y[n-1] should be zero, so output = 0
    REQUIRE(zero == Catch::Approx(0.0f).margin(1e-6f));
}

TEST_CASE("test_process_zero_samples_no_crash", "[DCFilter]")
{
    // Call process(buf, 0) after prepare() — the for-loop must not execute,
    // internal state (xPrev, yPrev) must remain at their initial values,
    // and subsequent real processing must still produce correct DC removal.
    DCFilter filter;
    filter.prepare(kSampleRate);

    // Process 0 samples — must not crash and must not touch state
    float dummy = 0.0f;
    filter.process(&dummy, 0);

    // State should be unchanged (both still 0.0f after prepare/reset).
    // Verify by processing a known DC signal and confirming DC is removed.
    const int numSamples = static_cast<int>(kSampleRate); // 1 second
    std::vector<float> signal(numSamples, 0.5f);           // pure DC offset

    filter.process(signal.data(), numSamples);

    // Measure DC in the latter half (filter settled)
    double sum = 0.0;
    const int start = numSamples / 2;
    for (int i = start; i < numSamples; ++i)
        sum += signal[i];
    const double dc = sum / (numSamples - start);

    // DC should be near zero — state was not corrupted by the zero-length call
    REQUIRE(std::abs(dc) < 1e-3);
}

TEST_CASE("test_high_samplerate_dc_removed_192khz", "[DCFilter]")
{
    constexpr double sr = 192000.0;
    DCFilter filter;
    filter.prepare(sr);

    const int numSamples = static_cast<int>(sr); // 1 second at 192 kHz
    std::vector<float> signal(numSamples, 0.5f); // pure DC

    filter.process(signal.data(), numSamples);

    // Measure DC in the latter half — should be removed
    double sum = 0.0;
    const int start = numSamples / 2;
    for (int i = start; i < numSamples; ++i)
        sum += signal[i];
    double dc = sum / (numSamples - start);

    REQUIRE(std::abs(dc) < 1e-3);
}

TEST_CASE("test_reprepare_clears_state", "[DCFilter]")
{
    DCFilter filter;
    filter.prepare(44100.0);

    // Prime the filter with DC=1.0
    std::vector<float> dc(1000, 1.0f);
    filter.process(dc.data(), static_cast<int>(dc.size()));

    // Re-prepare at different sample rate — should flush state
    filter.prepare(48000.0);

    // Process 100 zero samples
    std::vector<float> zeros(100, 0.0f);
    filter.process(zeros.data(), static_cast<int>(zeros.size()));

    // Last output should be ~0 since state was cleared and input is zero
    REQUIRE(std::abs(zeros[99]) < 1e-5f);
}

TEST_CASE("test_denormal_after_silence", "[DCFilter]")
{
    // Process 10000 samples of silence and verify outputs are not denormal.
    // This exercises the denormal protection added to yPrev.
    DCFilter filter;
    filter.prepare(kSampleRate);

    // Prime the filter with a small signal to push state away from zero
    std::vector<float> prime(100, 1e-10f);
    filter.process(prime.data(), static_cast<int>(prime.size()));

    // Process 10000 samples of silence
    std::vector<float> silence(10000, 0.0f);
    filter.process(silence.data(), static_cast<int>(silence.size()));

    // All output samples should be either zero or normal (not denormal)
    for (int i = 0; i < 10000; ++i)
    {
        float v = silence[i];
        bool isNormalOrZero = (v == 0.0f) || std::isnormal(v);
        REQUIRE(isNormalOrZero);
    }

    // After silence, processing one more sample should also produce a non-denormal
    float probe = 0.5f;
    filter.process(&probe, 1);
    REQUIRE((probe == 0.0f || std::isnormal(probe)));
}

TEST_CASE("test_single_sample_blocks", "[DCFilter]")
{
    constexpr double sr = 44100.0;
    DCFilter filter;
    filter.prepare(sr);

    const int numSamples = static_cast<int>(sr); // 1 second
    std::vector<float> output(numSamples);

    // Process one sample at a time with DC=0.5
    for (int i = 0; i < numSamples; ++i)
    {
        output[i] = 0.5f;
        filter.process(&output[i], 1);
    }

    // Measure DC in the latter half — should be removed
    double sum = 0.0;
    const int start = numSamples / 2;
    for (int i = start; i < numSamples; ++i)
        sum += output[i];
    double dc = sum / (numSamples - start);

    REQUIRE(std::abs(dc) < 1e-3);
}

TEST_CASE("test_cutoff_near_5hz_minus3db", "[DCFilter]")
{
    DCFilter filter;
    filter.prepare(kSampleRate);

    // 2 seconds = 10 cycles of 5 Hz; pole at R≈0.9993 settles in ~6600 samples
    const int numSamples = 88200;
    const double freq = 5.0;
    std::vector<float> signal(numSamples);
    for (int i = 0; i < numSamples; ++i)
        signal[i] = static_cast<float>(std::sin(kTwoPi * freq * i / kSampleRate));

    filter.process(signal.data(), numSamples);

    // Measure RMS of latter half (steady state, 5 cycles)
    double sumSq = 0.0;
    const int start = numSamples / 2;
    for (int i = start; i < numSamples; ++i)
        sumSq += signal[i] * signal[i];
    double rmsOut = std::sqrt(sumSq / (numSamples - start));

    double rmsIn = std::sqrt(0.5); // unit sine RMS = 1/sqrt(2)
    double attenuationDb = 20.0 * std::log10(rmsOut / rmsIn);

    // First-order HP at cutoff → -3 dB; tolerance ±1.5 dB
    REQUIRE(attenuationDb > -4.5);
    REQUIRE(attenuationDb < -1.5);
}

TEST_CASE("test_1hz_heavily_attenuated", "[DCFilter]")
{
    DCFilter filter;
    filter.prepare(kSampleRate);

    // 2 seconds = 2 cycles of 1 Hz; filter settles well before the midpoint
    const int numSamples = 88200;
    const double freq = 1.0;
    std::vector<float> signal(numSamples);
    for (int i = 0; i < numSamples; ++i)
        signal[i] = static_cast<float>(std::sin(kTwoPi * freq * i / kSampleRate));

    filter.process(signal.data(), numSamples);

    // Measure RMS of latter half (1 full cycle of 1 Hz)
    double sumSq = 0.0;
    const int start = numSamples / 2;
    for (int i = start; i < numSamples; ++i)
        sumSq += signal[i] * signal[i];
    double rmsOut = std::sqrt(sumSq / (numSamples - start));

    double rmsIn = std::sqrt(0.5);
    double attenuationDb = 20.0 * std::log10(rmsOut / rmsIn);

    // First-order HP at 1/5 of cutoff → ~-14 dB; require > 12 dB attenuation
    REQUIRE(attenuationDb < -12.0);
}

TEST_CASE("test_20hz_passes_with_little_attenuation", "[DCFilter]")
{
    DCFilter filter;
    filter.prepare(kSampleRate);

    // 1 second = 20 cycles of 20 Hz; well above cutoff
    const int numSamples = 44100;
    const double freq = 20.0;
    std::vector<float> signal(numSamples);
    for (int i = 0; i < numSamples; ++i)
        signal[i] = static_cast<float>(std::sin(kTwoPi * freq * i / kSampleRate));

    filter.process(signal.data(), numSamples);

    // Measure RMS of latter half (10 full cycles)
    double sumSq = 0.0;
    const int start = numSamples / 2;
    for (int i = start; i < numSamples; ++i)
        sumSq += signal[i] * signal[i];
    double rmsOut = std::sqrt(sumSq / (numSamples - start));

    double rmsIn = std::sqrt(0.5);
    double attenuationDb = 20.0 * std::log10(rmsOut / rmsIn);

    // At 4× the cutoff, first-order HP attenuates by only ~-0.26 dB
    REQUIRE(std::abs(attenuationDb) < 1.5);
}

TEST_CASE("test_cutoff_scales_with_sample_rate", "[DCFilter]")
{
    constexpr double sr = 96000.0;
    DCFilter filter;
    filter.prepare(sr);

    // 2 seconds at 96 kHz = 192000 samples = 10 cycles of 5 Hz
    const int numSamples = 192000;
    const double freq = 5.0;
    std::vector<float> signal(numSamples);
    for (int i = 0; i < numSamples; ++i)
        signal[i] = static_cast<float>(std::sin(kTwoPi * freq * i / sr));

    filter.process(signal.data(), numSamples);

    // Measure RMS of latter half (5 cycles of 5 Hz)
    double sumSq = 0.0;
    const int start = numSamples / 2;
    for (int i = start; i < numSamples; ++i)
        sumSq += signal[i] * signal[i];
    double rmsOut = std::sqrt(sumSq / (numSamples - start));

    double rmsIn = std::sqrt(0.5);
    double attenuationDb = 20.0 * std::log10(rmsOut / rmsIn);

    // R recomputed for 96 kHz: first-order HP at cutoff → -3 dB; tolerance ±1.5 dB
    REQUIRE(attenuationDb > -4.5);
    REQUIRE(attenuationDb < -1.5);
}
