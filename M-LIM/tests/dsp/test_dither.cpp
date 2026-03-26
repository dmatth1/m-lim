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

TEST_CASE("test_noise_shaping_stable_44k", "[Dither]")
{
    // Weighted mode at 44.1 kHz must not diverge over a long run of silence.
    Dither d;
    d.prepare(44100.0);
    d.setBitDepth(16);
    d.setNoiseShaping(2);

    const int numSamples = 100000;
    std::vector<float> signal(numSamples, 0.0f);
    d.process(signal.data(), numSamples);

    const float step = std::pow(2.0f, 1.0f - 16.0f);
    const float twoLSB = 2.0f * step;

    for (int i = 0; i < numSamples; ++i)
    {
        // All output samples must be finite.
        REQUIRE(std::isfinite(signal[i]));
        // With stable feedback the quantisation error stays bounded.
        // Output of silence through dither is bounded to a few LSBs.
        REQUIRE(std::abs(signal[i]) <= twoLSB * 10.0f);
    }
}

TEST_CASE("test_noise_shaping_stable_48k", "[Dither]")
{
    Dither d;
    d.prepare(48000.0);
    d.setBitDepth(16);
    d.setNoiseShaping(2);

    const int numSamples = 100000;
    std::vector<float> signal(numSamples, 0.0f);
    d.process(signal.data(), numSamples);

    const float step = std::pow(2.0f, 1.0f - 16.0f);
    const float twoLSB = 2.0f * step;

    for (int i = 0; i < numSamples; ++i)
    {
        REQUIRE(std::isfinite(signal[i]));
        REQUIRE(std::abs(signal[i]) <= twoLSB * 10.0f);
    }
}

TEST_CASE("test_noise_shaping_stable_96k", "[Dither]")
{
    // At >=88.2 kHz coefficients are zeroed; the loop is trivially stable.
    Dither d;
    d.prepare(96000.0);
    d.setBitDepth(16);
    d.setNoiseShaping(2);

    const int numSamples = 100000;
    std::vector<float> signal(numSamples, 0.0f);
    d.process(signal.data(), numSamples);

    const float step = std::pow(2.0f, 1.0f - 16.0f);

    for (int i = 0; i < numSamples; ++i)
    {
        REQUIRE(std::isfinite(signal[i]));
        // Zero feedback: output bounded to ±1 LSB.
        REQUIRE(std::abs(signal[i]) <= step + step * 0.01f);
    }
}

TEST_CASE("test_bit_depth_change_16_to_24_no_nan", "[Dither]")
{
    // Process at 16-bit, then switch to 24-bit mid-stream.
    // All output samples must be finite and within [-1, 1].
    Dither dither;
    dither.prepare(44100.0);
    dither.setBitDepth(16);
    dither.setNoiseShaping(1); // Optimized (first-order feedback)

    const int numSamples = 1000;
    std::vector<float> signal(numSamples);
    for (int i = 0; i < numSamples; ++i)
        signal[i] = 0.5f * std::sin(2.0f * 3.14159265f * 440.0f * static_cast<float>(i) / 44100.0f);

    dither.process(signal.data(), numSamples);

    // Now switch bit depth mid-session
    dither.setBitDepth(24);

    std::vector<float> signal2(numSamples);
    for (int i = 0; i < numSamples; ++i)
        signal2[i] = 0.5f * std::sin(2.0f * 3.14159265f * 440.0f * static_cast<float>(i + numSamples) / 44100.0f);

    dither.process(signal2.data(), numSamples);

    for (int i = 0; i < numSamples; ++i)
    {
        REQUIRE(std::isfinite(signal2[i]));
        REQUIRE(signal2[i] >= -1.0f);
        REQUIRE(signal2[i] <= 1.0f);
    }
}

TEST_CASE("test_bit_depth_change_24_to_16_no_nan", "[Dither]")
{
    // Process at 24-bit, then switch to 16-bit mid-stream.
    // All output samples must be finite and within [-1, 1].
    Dither dither;
    dither.prepare(44100.0);
    dither.setBitDepth(24);
    dither.setNoiseShaping(1); // Optimized (first-order feedback)

    const int numSamples = 1000;
    std::vector<float> signal(numSamples);
    for (int i = 0; i < numSamples; ++i)
        signal[i] = 0.5f * std::sin(2.0f * 3.14159265f * 440.0f * static_cast<float>(i) / 44100.0f);

    dither.process(signal.data(), numSamples);

    // Now switch bit depth mid-session
    dither.setBitDepth(16);

    std::vector<float> signal2(numSamples);
    for (int i = 0; i < numSamples; ++i)
        signal2[i] = 0.5f * std::sin(2.0f * 3.14159265f * 440.0f * static_cast<float>(i + numSamples) / 44100.0f);

    dither.process(signal2.data(), numSamples);

    for (int i = 0; i < numSamples; ++i)
    {
        REQUIRE(std::isfinite(signal2[i]));
        REQUIRE(signal2[i] >= -1.0f);
        REQUIRE(signal2[i] <= 1.0f);
    }
}

TEST_CASE("test_bit_depth_change_quantization_correct_after", "[Dither]")
{
    // After switching from 16-bit to 24-bit, outputs must be quantized to 24-bit steps.
    Dither dither;
    dither.prepare(44100.0);
    dither.setBitDepth(16);
    dither.setNoiseShaping(0); // Basic — no error feedback, easiest to verify step alignment

    const int numSamples = 512;
    std::vector<float> signal(numSamples);
    for (int i = 0; i < numSamples; ++i)
        signal[i] = -0.5f + static_cast<float>(i) / static_cast<float>(numSamples - 1);

    dither.process(signal.data(), numSamples);

    // Switch to 24-bit
    dither.setBitDepth(24);

    const float step24    = std::pow(2.0f, 1.0f - 24.0f);
    const float epsilon24 = step24 * 0.01f;

    std::vector<float> signal2(numSamples);
    for (int i = 0; i < numSamples; ++i)
        signal2[i] = -0.5f + static_cast<float>(i) / static_cast<float>(numSamples - 1);

    dither.process(signal2.data(), numSamples);

    // All samples after the bit-depth change should be quantized to 24-bit boundaries.
    for (int i = 0; i < numSamples; ++i)
        REQUIRE(isQuantized(signal2[i], step24, epsilon24));
}

TEST_CASE("test_second_order_dc_null_44k", "[Dither]")
{
    // With a near-DC-null shaping filter, the mean of the quantisation error at DC
    // approaches zero.  Process constant 0.5f; the mean of the output must match
    // the input mean to within 0.01 LSB (at 16-bit).
    Dither d;
    d.prepare(44100.0);
    d.setBitDepth(16);
    d.setNoiseShaping(2);

    const int numSamples = 8192;
    std::vector<float> signal(numSamples, 0.5f);
    d.process(signal.data(), numSamples);

    const float step = std::pow(2.0f, 1.0f - 16.0f);
    const float threshold = 0.01f * step;

    double sum = 0.0;
    for (int i = 0; i < numSamples; ++i)
        sum += static_cast<double>(signal[i]);
    float meanOutput = static_cast<float>(sum / numSamples);

    REQUIRE(std::abs(meanOutput - 0.5f) < threshold);
}

TEST_CASE("test_second_order_dc_null_48k", "[Dither]")
{
    // Same DC-null test at 48 kHz.
    Dither d;
    d.prepare(48000.0);
    d.setBitDepth(16);
    d.setNoiseShaping(2);

    const int numSamples = 8192;
    std::vector<float> signal(numSamples, 0.5f);
    d.process(signal.data(), numSamples);

    const float step = std::pow(2.0f, 1.0f - 16.0f);
    const float threshold = 0.01f * step;

    double sum = 0.0;
    for (int i = 0; i < numSamples; ++i)
        sum += static_cast<double>(signal[i]);
    float meanOutput = static_cast<float>(sum / numSamples);

    REQUIRE(std::abs(meanOutput - 0.5f) < threshold);
}

TEST_CASE("test_process_without_prepare_finite_output", "[Dither]")
{
    // Verify that calling process() on a freshly-constructed Dither (without
    // calling prepare()) produces finite output.  This exercises the zero-
    // initialisation of mError1 and mError2 in the constructor; if they held
    // undefined values the output could contain NaN or Inf.
    Dither dither;  // no prepare() call

    const int numSamples = 64;
    std::vector<float> signal(numSamples);
    for (int i = 0; i < numSamples; ++i)
        signal[i] = 0.5f * std::sin(2.0f * 3.14159265f * 440.0f * static_cast<float>(i) / 44100.0f);

    dither.process(signal.data(), numSamples);

    for (int i = 0; i < numSamples; ++i)
        REQUIRE(std::isfinite(signal[i]));
}

TEST_CASE("test_invalid_bitdepth_no_nan", "[Dither]")
{
    // setBitDepth() does not clamp: it computes step = 2^(1-bits) for any input.
    // For bits=0 → step=2.0, bits=-1 → step=4.0, bits=33 → step≈2.3e-10.
    // All are valid floats; output must remain finite (no NaN/Inf).

    const int numSamples = 512;
    std::vector<float> signal(numSamples);
    for (int i = 0; i < numSamples; ++i)
        signal[i] = 0.5f * std::sin(2.0f * 3.14159265f * 440.0f * static_cast<float>(i) / 44100.0f);

    for (int invalidBits : {0, -1, 33})
    {
        Dither dither;
        dither.prepare(kSampleRate);
        dither.setBitDepth(invalidBits);
        dither.setNoiseShaping(0);

        std::vector<float> buf(signal);
        dither.process(buf.data(), numSamples);

        for (int i = 0; i < numSamples; ++i)
            REQUIRE(std::isfinite(buf[i]));
    }
}

TEST_CASE("test_silence_input_dither_noise_bounded", "[Dither]")
{
    // With 16-bit depth and silence input, TPDF dither should add low-level noise:
    //   - RMS > 0 (dither is actually added)
    //   - RMS < 2 LSBs (bounded; 1 LSB at 16-bit = 2^(1-16) = 1/32768 ≈ 3.05e-5)

    Dither dither;
    dither.prepare(kSampleRate);
    dither.setBitDepth(16);
    dither.setNoiseShaping(0); // Basic: no error feedback, pure TPDF

    const int numSamples = 1024;
    std::vector<float> signal(numSamples, 0.0f);
    dither.process(signal.data(), numSamples);

    double sumSq = 0.0;
    for (float s : signal)
        sumSq += static_cast<double>(s) * static_cast<double>(s);
    const float rms = static_cast<float>(std::sqrt(sumSq / numSamples));

    const float oneLSB = std::pow(2.0f, 1.0f - 16.0f); // ~3.05e-5
    const float twoLSB = 2.0f * oneLSB;                 // ~6.10e-5

    REQUIRE(rms > 0.0f);     // dither adds noise to silence
    REQUIRE(rms < twoLSB);   // bounded to 2 LSBs
}

TEST_CASE("test_input_at_ceiling_no_crash", "[Dither]")
{
    // Process 1024 samples at exactly 1.0f with 24-bit depth and noise shaping mode 1.
    // With TPDF dither added, output could theoretically exceed 1.0, but must be finite.

    Dither dither;
    dither.prepare(kSampleRate);
    dither.setBitDepth(24);
    dither.setNoiseShaping(1); // Optimized first-order feedback

    const int numSamples = 1024;
    std::vector<float> signal(numSamples, 1.0f);
    dither.process(signal.data(), numSamples);

    for (int i = 0; i < numSamples; ++i)
        REQUIRE(std::isfinite(signal[i]));
}

TEST_CASE("test_32bit_depth_quantization_minimal_noise", "[Dither]")
{
    // At 32-bit depth the quantization step is 2^(1-32) ≈ 4.65e-10.
    // TPDF dither noise is bounded to ±step, so deviation from input is negligible.
    // For an input of 0.5f, all output samples must stay within 1e-6 of 0.5f.

    Dither dither;
    dither.prepare(kSampleRate);
    dither.setBitDepth(32);
    dither.setNoiseShaping(0);

    const int numSamples = 1000;
    std::vector<float> signal(numSamples, 0.5f);
    dither.process(signal.data(), numSamples);

    for (int i = 0; i < numSamples; ++i)
        REQUIRE(std::abs(signal[i] - 0.5f) < 1e-6f);
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
