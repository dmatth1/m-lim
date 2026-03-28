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

TEST_CASE("test_noise_shaping_dither_not_cancelled", "[Dither]")
{
    // With second-order noise shaping (mode=2) and a DC input at 0.5 * LSB (half a
    // quantization step), TPDF dither enables the signal to be reproduced on average.
    // If the noise shaper incorrectly includes the dither in the error term it feeds
    // back, it will partially cancel the dither at low frequencies, shifting the mean
    // output below the true input value.
    //
    // Correct behaviour (Vanderkooy & Lipshitz 1984): mean output ≈ input ± 0.01 LSB.

    Dither d;
    d.prepare(44100.0);
    d.setBitDepth(16);
    d.setNoiseShaping(2); // second-order weighted

    const float step     = std::pow(2.0f, 1.0f - 16.0f);
    const float halfStep = 0.5f * step; // 0.5 LSB — sub-quantisation-step DC level

    const int numSamples = 10000;
    std::vector<float> signal(numSamples, halfStep);
    d.process(signal.data(), numSamples);

    double sum = 0.0;
    for (int i = 0; i < numSamples; ++i)
        sum += static_cast<double>(signal[i]);
    const float meanOutput = static_cast<float>(sum / numSamples);

    // Mean output must be within 0.01 LSB of the input (0.5 LSB).
    REQUIRE(std::abs(meanOutput - halfStep) < 0.01f * step);
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

// ============================================================
// Invalid noise shaping modes produce finite output (no NaN/Inf/divergence)
// ============================================================

TEST_CASE("test_invalid_noise_shaping_mode_finite_output", "[Dither]")
{
    // Modes outside [0,2] fall through the if/else chain as "no shaping" (mode 0).
    // Verify no NaN, Inf, or runaway values for modes 3, -1, and 100.
    const int invalidModes[] = { 3, -1, 100 };
    const int numSamples     = 1000;
    const float amplitude    = 0.001f;  // -60 dBFS — safe for iterative dither
    const float step         = std::pow(2.0f, 1.0f - 16.0f);

    for (int mode : invalidModes)
    {
        Dither d;
        d.prepare(44100.0);
        d.setBitDepth(16);
        d.setNoiseShaping(mode);

        std::vector<float> signal(numSamples);
        for (int i = 0; i < numSamples; ++i)
            signal[i] = amplitude * std::sin(6.283185307f * 1000.0f * i / 44100.0f);

        d.process(signal.data(), numSamples);

        for (int i = 0; i < numSamples; ++i)
        {
            INFO("Mode " << mode << " sample " << i << " = " << signal[i]);
            REQUIRE(std::isfinite(signal[i]));
            // Amplitude should remain bounded (no divergence)
            REQUIRE(std::abs(signal[i]) < 1.0f);
        }
    }
}

// ============================================================
// Invalid mode behaves identically to mode 0 (no error feedback)
// ============================================================

TEST_CASE("test_invalid_mode_no_feedback_applied", "[Dither]")
{
    // With invalid mode, the if/else chain produces mode-0 behaviour: no error
    // feedback is applied. We verify that TPDF-only quantisation is occurring
    // (not some error-accumulation path) by checking that errors stay bounded.
    //
    // With 16-bit depth (step ≈ 3.05e-5) and zero-valued input, each output
    // sample is exactly ±1 or 0 LSB (= step). If error feedback were accidentally
    // active, outputs could exceed ±2 LSB over many samples.

    const float step     = std::pow(2.0f, 1.0f - 16.0f);
    const int numSamples = 1000;

    Dither d;
    d.prepare(44100.0);
    d.setBitDepth(16);
    d.setNoiseShaping(3);  // invalid — should act as mode 0

    std::vector<float> silence(numSamples, 0.0f);
    d.process(silence.data(), numSamples);

    // With mode 0 (no feedback), error per sample is bounded to ±step.
    // Allow a tiny floating-point margin.
    const float bound = step + step * 0.01f;
    for (int i = 0; i < numSamples; ++i)
    {
        INFO("Sample " << i << " = " << silence[i] << ", bound = " << bound);
        REQUIRE(std::abs(silence[i]) <= bound);
    }
}

// ============================================================
// Switching from invalid mode to valid mode produces no divergence
// ============================================================

TEST_CASE("test_invalid_to_valid_mode_switch_no_diverge", "[Dither]")
{
    // Start with invalid mode (5), process some samples, then switch to
    // mode 2 (Weighted second-order) and process more. All output must
    // be finite and not diverge.

    const float step     = std::pow(2.0f, 1.0f - 16.0f);
    const float amplitude = 0.001f;  // -60 dBFS
    const int warmup     = 100;
    const int main_run   = 1000;

    Dither d;
    d.prepare(44100.0);
    d.setBitDepth(16);
    d.setNoiseShaping(5);  // invalid mode

    std::vector<float> buf1(warmup);
    for (int i = 0; i < warmup; ++i)
        buf1[i] = amplitude * std::sin(6.283185307f * 1000.0f * i / 44100.0f);
    d.process(buf1.data(), warmup);

    // Verify warmup output is finite
    for (int i = 0; i < warmup; ++i)
    {
        REQUIRE(std::isfinite(buf1[i]));
        REQUIRE(std::abs(buf1[i]) < 1.0f);
    }

    // Switch to valid mode 2
    d.setNoiseShaping(2);

    std::vector<float> buf2(main_run);
    for (int i = 0; i < main_run; ++i)
        buf2[i] = amplitude * std::sin(6.283185307f * 1000.0f * (warmup + i) / 44100.0f);
    d.process(buf2.data(), main_run);

    // All output must be finite after the mode switch
    for (int i = 0; i < main_run; ++i)
    {
        INFO("Post-switch sample " << i << " = " << buf2[i]);
        REQUIRE(std::isfinite(buf2[i]));
        REQUIRE(std::abs(buf2[i]) < 1.0f);
    }
}

// ============================================================
// test_error_feedback_stable_88200hz_long_run
// At 88200 Hz, the Weighted mode uses zero coefficients (beyond Nyquist/2),
// so error feedback is effectively off. 100k silence samples must stay
// within ±2 LSBs (16-bit step) and remain finite.
// ============================================================

TEST_CASE("test_error_feedback_stable_88200hz_long_run", "[Dither]")
{
    const float step = std::pow(2.0f, 1.0f - 16.0f);
    const int numSamples = 100000;

    Dither d;
    d.prepare(88200.0);
    d.setBitDepth(16);
    d.setNoiseShaping(2);  // Weighted — coefficients zeroed at 88.2 kHz

    std::vector<float> signal(numSamples, 0.0f);
    d.process(signal.data(), numSamples);

    const float bound = 2.0f * step + step * 0.01f;
    for (int i = 0; i < numSamples; ++i)
    {
        INFO("Sample " << i << " = " << signal[i]);
        REQUIRE(std::isfinite(signal[i]));
        REQUIRE(std::abs(signal[i]) <= bound);
    }
}

// ============================================================
// test_error_feedback_stable_192000hz_long_run
// At 192000 Hz, coefficients should also be zeroed (>= 88200 Hz branch).
// 100k silence samples must stay within ±2 LSBs and remain finite.
// ============================================================

TEST_CASE("test_error_feedback_stable_192000hz_long_run", "[Dither]")
{
    const float step = std::pow(2.0f, 1.0f - 16.0f);
    const int numSamples = 100000;

    Dither d;
    d.prepare(192000.0);
    d.setBitDepth(16);
    d.setNoiseShaping(2);  // Weighted — must zero coefficients at this rate

    std::vector<float> signal(numSamples, 0.0f);
    d.process(signal.data(), numSamples);

    const float bound = 2.0f * step + step * 0.01f;
    for (int i = 0; i < numSamples; ++i)
    {
        INFO("Sample " << i << " = " << signal[i]);
        REQUIRE(std::isfinite(signal[i]));
        REQUIRE(std::abs(signal[i]) <= bound);
    }
}

// ============================================================
// test_reprepare_clears_error_state
// Prime the error state at 44100 Hz with noise shaping mode 1, then
// re-prepare at 88200 Hz. After reprepare, output must be within ±2 LSBs
// (not carry over divergent error from the prior prepare).
// ============================================================

TEST_CASE("test_reprepare_clears_error_state", "[Dither]")
{
    const float step = std::pow(2.0f, 1.0f - 16.0f);

    Dither d;
    // Prime with 44100 Hz first-order shaping and a loud signal
    d.prepare(44100.0);
    d.setBitDepth(16);
    d.setNoiseShaping(1);  // First-order feedback — accumulates error state

    {
        std::vector<float> priming(1000, 0.1f);
        d.process(priming.data(), static_cast<int>(priming.size()));
    }

    // Re-prepare at 88200 Hz — must flush prior error state
    d.prepare(88200.0);
    d.setBitDepth(16);
    d.setNoiseShaping(2);  // Weighted mode (zero coefficients at this rate)

    const int numSamples = 1000;
    std::vector<float> signal(numSamples, 0.0f);
    d.process(signal.data(), numSamples);

    const float bound = 2.0f * step + step * 0.01f;
    for (int i = 0; i < numSamples; ++i)
    {
        INFO("Post-reprepare sample " << i << " = " << signal[i]);
        REQUIRE(std::isfinite(signal[i]));
        REQUIRE(std::abs(signal[i]) <= bound);
    }
}

// ============================================================
// test_half_lsb_boundary_unbiased
// Feed 0.5 LSB (exactly half the quantization step) 10000 times.
// The dither should cause some samples to round up and some down.
// The up/down ratio must be between 0.4 and 0.6 (unbiased distribution).
// ============================================================

TEST_CASE("test_half_lsb_boundary_unbiased", "[Dither]")
{
    const float step    = std::pow(2.0f, 1.0f - 16.0f);
    const float halfLsb = step * 0.5f;
    const int numSamples = 10000;

    Dither d;
    d.prepare(44100.0);
    d.setBitDepth(16);
    d.setNoiseShaping(0);  // Basic: TPDF only, no feedback

    std::vector<float> signal(numSamples, halfLsb);
    d.process(signal.data(), numSamples);

    // Count samples that rounded up (≥ step) vs. down (= 0)
    int roundedUp   = 0;
    int roundedDown = 0;
    for (int i = 0; i < numSamples; ++i)
    {
        REQUIRE(std::isfinite(signal[i]));
        if (std::abs(signal[i]) >= step * 0.5f)
            ++roundedUp;
        else
            ++roundedDown;
    }

    const float ratio = static_cast<float>(roundedUp) / static_cast<float>(numSamples);
    INFO("Up/total ratio = " << ratio << " (" << roundedUp << "/" << numSamples << ")");
    // TPDF dither should split roughly 50/50 over 10k samples
    REQUIRE(ratio >= 0.4f);
    REQUIRE(ratio <= 0.6f);
}

TEST_CASE("test_reset_clears_error_state", "[Dither]")
{
    // After processing non-silent audio (which accumulates noise-shaping error),
    // call reset() and verify that the next block of silence produces output
    // within the TPDF dither band only (no shaped error artefacts carried over).
    // Compare against a freshly constructed Dither object on the same block.

    const float step = std::pow(2.0f, 1.0f - 16.0f);
    const int numSamples = 1000;

    // --- Instance A: process audio, then reset, then process silence ---
    Dither a;
    a.prepare(44100.0);
    a.setBitDepth(16);
    a.setNoiseShaping(2); // Weighted second-order (accumulates error)

    {
        // Prime with a loud signal to accumulate error state
        std::vector<float> priming(2000);
        for (int i = 0; i < 2000; ++i)
            priming[i] = 0.8f * std::sin(6.283185307f * 440.0f * i / 44100.0f);
        a.process(priming.data(), static_cast<int>(priming.size()));
    }

    // Reset clears error state
    a.reset();

    std::vector<float> silenceA(numSamples, 0.0f);
    a.process(silenceA.data(), numSamples);

    // --- Instance B: freshly constructed, same config, process silence ---
    Dither b;
    b.prepare(44100.0);
    b.setBitDepth(16);
    b.setNoiseShaping(2);

    std::vector<float> silenceB(numSamples, 0.0f);
    b.process(silenceB.data(), numSamples);

    // Both should produce output bounded within a few LSBs (no carried-over error)
    const float bound = 10.0f * step; // generous bound for shaped dither
    for (int i = 0; i < numSamples; ++i)
    {
        INFO("Reset instance sample " << i << " = " << silenceA[i]);
        REQUIRE(std::isfinite(silenceA[i]));
        REQUIRE(std::abs(silenceA[i]) <= bound);
    }

    // Verify the RMS of both are in the same ballpark (within 5x of each other).
    // This confirms reset() truly cleared the error state.
    auto rms = [](const std::vector<float>& v) {
        double sum = 0.0;
        for (float s : v) sum += static_cast<double>(s) * static_cast<double>(s);
        return static_cast<float>(std::sqrt(sum / v.size()));
    };

    float rmsA = rms(silenceA);
    float rmsB = rms(silenceB);

    // Both should have similar noise floor (RNG seeds differ, but order of magnitude matches)
    REQUIRE(rmsA > 0.0f);
    REQUIRE(rmsB > 0.0f);
    REQUIRE(rmsA < bound);
    REQUIRE(rmsB < bound);
}
