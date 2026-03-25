#include "catch2/catch_amalgamated.hpp"
#include "dsp/Dither.h"
#include <cmath>
#include <vector>
#include <numeric>

static constexpr double kSampleRate = 44100.0;

// Returns true if 'value' is within 'epsilon' of a multiple of 'step'.
static bool isQuantized(float value, float step, float epsilon)
{
    float remainder = std::fmod(std::abs(value), step);
    return remainder < epsilon || remainder > step - epsilon;
}

TEST_CASE("test_16bit_quantization", "[Dither]")
{
    Dither dither;
    dither.prepare(kSampleRate);
    dither.setBitDepth(16);
    dither.setNoiseShaping(0); // Basic — no feedback, easiest to verify

    const float step    = std::pow(2.0f, 1.0f - 16.0f); // ~3.05e-5
    const float epsilon = step * 0.01f;                  // 1% of step

    // Use a simple ramp that exercises many quantisation levels.
    const int numSamples = 1024;
    std::vector<float> signal(numSamples);
    for (int i = 0; i < numSamples; ++i)
        signal[i] = -1.0f + 2.0f * static_cast<float>(i) / static_cast<float>(numSamples - 1);

    dither.process(signal.data(), numSamples);

    for (int i = 0; i < numSamples; ++i)
        REQUIRE(isQuantized(signal[i], step, epsilon));
}

TEST_CASE("test_24bit_quantization", "[Dither]")
{
    Dither dither;
    dither.prepare(kSampleRate);
    dither.setBitDepth(24);
    dither.setNoiseShaping(0);

    const float step    = std::pow(2.0f, 1.0f - 24.0f); // ~1.19e-7
    const float epsilon = step * 0.01f;

    const int numSamples = 512;
    std::vector<float> signal(numSamples);
    for (int i = 0; i < numSamples; ++i)
        signal[i] = -0.5f + static_cast<float>(i) / static_cast<float>(numSamples - 1);

    dither.process(signal.data(), numSamples);

    for (int i = 0; i < numSamples; ++i)
        REQUIRE(isQuantized(signal[i], step, epsilon));
}

TEST_CASE("test_dither_adds_noise", "[Dither]")
{
    // Feed a zero signal.  Without dither, every output would be 0.
    // With TPDF dither, some samples should be pushed to ±step, so the
    // output should not be identically zero across all samples.

    Dither dither;
    dither.prepare(kSampleRate);
    dither.setBitDepth(16);
    dither.setNoiseShaping(0);

    const int numSamples = 4096;
    std::vector<float> signal(numSamples, 0.0f); // all zeros

    dither.process(signal.data(), numSamples);

    // At least some samples should be non-zero (dither noise).
    int nonZeroCount = 0;
    for (float s : signal)
        if (s != 0.0f) ++nonZeroCount;

    // With TPDF dither the expected non-zero rate is ~25% (1 - P(triangular in centre step)).
    // Use a conservative threshold of 10% to avoid spurious failures from RNG variance.
    REQUIRE(nonZeroCount > numSamples / 10);
}

TEST_CASE("test_noise_shaping_modes", "[Dither]")
{
    // All three modes should produce different output for the same input,
    // because modes 1 and 2 add error feedback before quantising.

    const int numSamples = 1024;
    std::vector<float> src(numSamples);
    for (int i = 0; i < numSamples; ++i)
        src[i] = 0.3f * std::sin(2.0f * 3.14159265f * 440.0f * static_cast<float>(i) / static_cast<float>(kSampleRate));

    auto runMode = [&](int mode) -> std::vector<float> {
        // Each mode gets an identical seeded Dither instance.
        Dither d;
        d.prepare(kSampleRate);
        d.setBitDepth(16);
        d.setNoiseShaping(mode);
        std::vector<float> buf(src);
        d.process(buf.data(), numSamples);
        return buf;
    };

    auto out0 = runMode(0);
    auto out1 = runMode(1);
    auto out2 = runMode(2);

    // Count samples that differ between modes.
    int diff01 = 0, diff02 = 0, diff12 = 0;
    for (int i = 0; i < numSamples; ++i)
    {
        if (out0[i] != out1[i]) ++diff01;
        if (out0[i] != out2[i]) ++diff02;
        if (out1[i] != out2[i]) ++diff12;
    }

    // Modes must produce meaningfully different outputs.
    REQUIRE(diff01 > 0);
    REQUIRE(diff02 > 0);
    REQUIRE(diff12 > 0);
}

TEST_CASE("test_noise_shaping_at_48000", "[Dither]")
{
    // Weighted mode (mode 2) uses different coefficients at 44.1kHz vs 48kHz.
    // Running the same signal through both should yield different outputs.

    const int numSamples = 1024;
    std::vector<float> src(numSamples);
    for (int i = 0; i < numSamples; ++i)
        src[i] = 0.3f * std::sin(2.0f * 3.14159265f * 440.0f * static_cast<float>(i) / 44100.0f);

    auto run = [&](double sr) -> std::vector<float> {
        Dither d;
        d.prepare(sr);
        d.setBitDepth(16);
        d.setNoiseShaping(2); // Weighted
        std::vector<float> buf(src);
        d.process(buf.data(), numSamples);
        return buf;
    };

    auto out441 = run(44100.0);
    auto out480 = run(48000.0);

    // Different coefficients at the two rates must produce different output.
    int diffCount = 0;
    for (int i = 0; i < numSamples; ++i)
        if (out441[i] != out480[i]) ++diffCount;

    REQUIRE(diffCount > 0);
}

TEST_CASE("test_high_sample_rate_fallback", "[Dither]")
{
    // At >=88.2kHz, Weighted mode (mode 2) uses zero noise-shaping coefficients.
    // With silence as input and no error feedback, each quantised sample is shifted
    // only by TPDF dither, so output is bounded to ±1 LSB (one quantisation step).
    // If error feedback were active (as at 44.1kHz), accumulated errors could push
    // output beyond ±1 LSB.

    const float step = std::pow(2.0f, 1.0f - 16.0f);

    Dither d;
    d.prepare(88200.0);
    d.setBitDepth(16);
    d.setNoiseShaping(2); // Weighted — should have zero coefficients at this rate

    const int numSamples = 4096;
    std::vector<float> signal(numSamples, 0.0f); // silence
    d.process(signal.data(), numSamples);

    // With zero feedback, no error can accumulate; output stays within ±1 LSB.
    const float epsilon = step * 0.01f;
    for (int i = 0; i < numSamples; ++i)
        REQUIRE(std::abs(signal[i]) <= step + epsilon);
}
