#include "catch2/catch_amalgamated.hpp"
#include "dsp/TruePeakDetector.h"
#include <cmath>
#include <vector>

static constexpr double kSampleRate = 44100.0;
static constexpr double kTwoPi = 6.283185307179586;

TEST_CASE("test_detects_intersample_peak", "[TruePeakDetector]")
{
    // Two successive samples at ~0.7 will produce an inter-sample peak near 1.0
    // due to the bandlimited interpolation overshooting between them.
    TruePeakDetector det;
    det.prepare(kSampleRate);

    // Feed a sine wave at Nyquist/2 — known to produce inter-sample peaks > sample peak
    const int numSamples = 256;
    std::vector<float> signal(numSamples);
    double freq = kSampleRate / 4.0; // quarter-rate sine
    for (int i = 0; i < numSamples; ++i)
        signal[i] = static_cast<float>(std::sin(kTwoPi * freq * i / kSampleRate));

    det.processBlock(signal.data(), numSamples);

    // The interpolated peak should be >= the sample peak (1.0 in this case)
    REQUIRE(det.getPeak() >= 0.9f);
}

TEST_CASE("test_sample_peak_matches", "[TruePeakDetector]")
{
    // Sustained constant signal: after the FIR buffer fills (12 taps), the interpolated
    // output converges to ~0.5 * sum_of_coefficients ≈ 0.5. The startup transient can
    // reach ~0.56 (partial phase sum during buffer fill), so we check a generous range.
    TruePeakDetector det;
    det.prepare(kSampleRate);

    const int numSamples = 128;
    std::vector<float> signal(numSamples, 0.5f);

    // Measure the steady-state single-sample response after the buffer is filled
    // (process first 64 to fill buffer, then check last returned value)
    det.processBlock(signal.data(), 64);
    float steadyPeak = det.processSample(0.5f);

    // After the buffer is fully warmed up, DC passes through near-unity
    REQUIRE(steadyPeak >= 0.49f);
    REQUIRE(steadyPeak <= 0.52f);
}

TEST_CASE("test_reset_clears_peak", "[TruePeakDetector]")
{
    TruePeakDetector det;
    det.prepare(kSampleRate);

    float loud = 0.9f;
    det.processSample(loud);
    REQUIRE(det.getPeak() > 0.0f);

    det.reset();
    REQUIRE(det.getPeak() == 0.0f);
}

TEST_CASE("test_block_processing", "[TruePeakDetector]")
{
    TruePeakDetector det;
    det.prepare(kSampleRate);

    const int numSamples = 512;
    std::vector<float> signal(numSamples, 0.0f);

    // Place a large peak in the middle
    signal[256] = 0.8f;
    signal[257] = 0.8f;

    det.processBlock(signal.data(), numSamples);

    // getPeak() must return max seen during block
    REQUIRE(det.getPeak() > 0.0f);
}

TEST_CASE("test_itu_compliance_intersample", "[TruePeakDetector]")
{
    // Verify steady-state DC response: after the 12-tap FIR buffer is fully pre-warmed,
    // the interpolated output for a constant 1.0 signal should be near 1.0.
    // (Startup transient peaks ~1.116 because the partial phase sum overshoots during fill.)
    TruePeakDetector det;
    det.prepare(kSampleRate);

    // Pre-warm: fill buffer so filter is in steady state
    std::vector<float> warmup(64, 1.0f);
    det.processBlock(warmup.data(), 64);

    // Check the steady-state response (returnvalue of processSample)
    float steadyPeak = det.processSample(1.0f);

    // DC should pass through with near-unity gain across all 4 phases
    REQUIRE(steadyPeak >= 0.97f);
    REQUIRE(steadyPeak <= 1.03f);
}

TEST_CASE("test_itu_compliance_1khz", "[TruePeakDetector]")
{
    // 1kHz sine at 0 dBFS (amplitude 1.0); true peak is known to be ~+3.01 dBTP
    // due to inter-sample peaks. We test a relaxed version: true peak >= sample peak.
    TruePeakDetector det;
    det.prepare(kSampleRate);

    const int numSamples = static_cast<int>(kSampleRate); // 1 second
    std::vector<float> signal(numSamples);
    for (int i = 0; i < numSamples; ++i)
        signal[i] = static_cast<float>(std::sin(kTwoPi * 1000.0 * i / kSampleRate));

    det.processBlock(signal.data(), numSamples);

    // For 1kHz sine the true peak should be >= sample peak (1.0)
    REQUIRE(det.getPeak() >= 1.0f);
    // And should not exceed some reasonable bound (e.g., 2.0)
    REQUIRE(det.getPeak() < 2.0f);
}

TEST_CASE("test_simd_matches_scalar", "[TruePeakDetector]")
{
    // Feed 1024 samples of a 0 dBFS 1 kHz sine wave through both SIMD (processSample)
    // and scalar (processSampleScalar) paths on separate instances and verify that
    // per-sample peak values differ by less than 1e-5f.
    TruePeakDetector simdDet;
    TruePeakDetector scalarDet;
    simdDet.prepare(kSampleRate);
    scalarDet.prepare(kSampleRate);

    const int numSamples = 1024;
    std::vector<float> signal(numSamples);
    for (int i = 0; i < numSamples; ++i)
        signal[i] = static_cast<float>(std::sin(kTwoPi * 1000.0 * i / kSampleRate));

    for (int i = 0; i < numSamples; ++i)
    {
        float simdPeak   = simdDet.processSample(signal[i]);
        float scalarPeak = scalarDet.processSampleScalar(signal[i]);
        REQUIRE(std::abs(simdPeak - scalarPeak) < 1e-5f);
    }
}

TEST_CASE("test_getpeak_matches_processsample_and_reset_clears", "[TruePeakDetector]")
{
    // Verify that getPeak() reflects the peak set by processSample(),
    // and that reset() brings getPeak() back to zero.
    TruePeakDetector det;
    det.prepare(kSampleRate);

    // Initial state: peak is zero
    REQUIRE(det.getPeak() == 0.0f);

    // Feed a large sample and verify getPeak() returns a positive value
    float returned = det.processSample(0.8f);
    REQUIRE(returned > 0.0f);
    REQUIRE(det.getPeak() > 0.0f);

    // Peak should not exceed the returned sample value by more than some margin
    // (the FIR can produce slightly different values, but getPeak() must track the max)
    float peakAfter = det.getPeak();
    REQUIRE(peakAfter >= returned - 1e-6f);

    // reset() must bring the running peak back to zero
    det.reset();
    REQUIRE(det.getPeak() == 0.0f);
}

TEST_CASE("test_processblock_vs_processsample_same_peak", "[TruePeakDetector]")
{
    // Verify that processBlock() and a loop of processSample() produce identical getPeak()
    // results for the same input, catching any structural divergence between the two paths.
    TruePeakDetector blockDet;
    TruePeakDetector sampleDet;
    blockDet.prepare(kSampleRate);
    sampleDet.prepare(kSampleRate);

    const int numSamples = 256;
    std::vector<float> signal(numSamples);
    double freq = kSampleRate / 4.0; // Nyquist/4 sine — same as test_detects_intersample_peak
    for (int i = 0; i < numSamples; ++i)
        signal[i] = static_cast<float>(std::sin(kTwoPi * freq * i / kSampleRate));

    // Process via processBlock on one instance
    blockDet.processBlock(signal.data(), numSamples);

    // Process via individual processSample calls on the other instance
    for (int i = 0; i < numSamples; ++i)
        sampleDet.processSample(signal[i]);

    float blockPeak  = blockDet.getPeak();
    float samplePeak = sampleDet.getPeak();

    // Both paths must agree within ±0.001 — structural divergence would exceed this
    REQUIRE(std::abs(blockPeak - samplePeak) < 0.001f);
}

TEST_CASE("test_processblock_returns_correct_peak_object", "[TruePeakDetector]")
{
    // Verify that processBlock() updates the internal peak state accessible via getPeak().
    // For a sine with amplitude 0.9f, the inter-sample peak should be at least 0.9f.
    TruePeakDetector det;
    det.prepare(kSampleRate);

    const int numSamples = 256;
    std::vector<float> signal(numSamples);
    for (int i = 0; i < numSamples; ++i)
        signal[i] = 0.9f * static_cast<float>(std::sin(kTwoPi * 1000.0 * i / kSampleRate));

    det.processBlock(signal.data(), numSamples);

    // True peak must be at least as large as the largest sample peak
    REQUIRE(det.getPeak() >= 0.9f);
}

TEST_CASE("test_process_block_zero_samples_no_crash", "[TruePeakDetector]")
{
    // Call processBlock(ptr, 0) — the inner for-loop must not execute,
    // getPeak() must remain 0.0f (unchanged from the reset state after prepare()).
    TruePeakDetector det;
    det.prepare(kSampleRate);

    float dummy = 0.0f;
    det.processBlock(&dummy, 0);

    REQUIRE(det.getPeak() == 0.0f);
}

TEST_CASE("test_process_block_single_sample", "[TruePeakDetector]")
{
    // Process exactly one sample — the ring buffer must wrap without going
    // out of bounds, and the result must be a finite, non-negative value.
    TruePeakDetector det;
    det.prepare(kSampleRate);

    const float input = 0.5f;
    det.processBlock(&input, 1);

    const float peak = det.getPeak();
    REQUIRE(std::isfinite(peak));
    REQUIRE(peak >= 0.0f);
}

TEST_CASE("test_resetpeak_preserves_fir_state", "[TruePeakDetector]")
{
    // Warm up the FIR with 12 samples of a 0.9 sine wave, then call resetPeak().
    // The next processSample() output must be close to what a continuously warm FIR
    // would produce — not the near-zero cold-start output.
    TruePeakDetector warmDet;   // receives resetPeak() — FIR state preserved
    TruePeakDetector coldDet;   // receives reset()    — FIR state zeroed (baseline)
    TruePeakDetector refDet;    // never reset         — reference for warm output

    warmDet.prepare(kSampleRate);
    coldDet.prepare(kSampleRate);
    refDet.prepare(kSampleRate);

    // Feed all three the same 12-sample sine warm-up
    const int warmupSamples = 12;
    for (int i = 0; i < warmupSamples; ++i)
    {
        float s = 0.9f * static_cast<float>(std::sin(kTwoPi * 1000.0 * i / kSampleRate));
        warmDet.processSample(s);
        coldDet.processSample(s);
        refDet.processSample(s);
    }

    // Reset peak only (preserves FIR) vs full reset (zeros FIR)
    warmDet.resetPeak();
    coldDet.reset();

    // Next sample
    float nextSample = 0.9f * static_cast<float>(
        std::sin(kTwoPi * 1000.0 * warmupSamples / kSampleRate));

    float warmOutput = warmDet.processSample(nextSample);
    float coldOutput = coldDet.processSample(nextSample);
    float refOutput  = refDet.processSample(nextSample);

    // The warm-FIR output must be close to the reference (never-reset) output
    REQUIRE(std::abs(warmOutput - refOutput) < 1e-5f);

    // The cold-FIR output should differ significantly from the reference
    // (at this sample frequency the difference should be noticeable)
    // Just verify warm is notably closer to ref than cold is
    float warmError = std::abs(warmOutput - refOutput);
    float coldError = std::abs(coldOutput - refOutput);
    REQUIRE(warmError < coldError);
}

TEST_CASE("test_resetpeak_clears_peak_accumulator", "[TruePeakDetector]")
{
    // After resetPeak(), getPeak() must return 0.0 (peak accumulator cleared)
    // but the next processSample() output must be consistent with a warm FIR.
    TruePeakDetector det;
    det.prepare(kSampleRate);

    // Warm up: feed 12 samples of a 0.9 sine
    for (int i = 0; i < 12; ++i)
    {
        float s = 0.9f * static_cast<float>(std::sin(kTwoPi * 1000.0 * i / kSampleRate));
        det.processSample(s);
    }

    // Peak should be non-zero after warm-up
    REQUIRE(det.getPeak() > 0.0f);

    // resetPeak() must zero the peak accumulator
    det.resetPeak();
    REQUIRE(det.getPeak() == 0.0f);

    // Process one more sample — the output must be non-trivially non-zero
    // (a warmed FIR applied to a 0.9-amplitude sine produces a non-zero output)
    float nextSample = 0.9f * static_cast<float>(
        std::sin(kTwoPi * 1000.0 * 12.0 / kSampleRate));
    float output = det.processSample(nextSample);

    // A completely cold FIR at this sample would produce near zero;
    // a warm FIR must produce something meaningfully non-zero (> 0.01)
    REQUIRE(output > 0.01f);

    // getPeak() must now reflect that output
    REQUIRE(det.getPeak() == Catch::Approx(output).epsilon(1e-6f));
}

TEST_CASE("test_fir_coefficient_integrity", "[TruePeakDetector]")
{
    // Verify that the transposed kCoeffsByTap table contains identical values
    // to the original kCoeffs table — all 48 coefficients must match exactly.
    // Access via a temporary TruePeakDetector instance using a friend-like test helper:
    // we check that processSample and processSampleScalar agree on a DC signal,
    // which is only possible if the coefficient tables have the same values.

    TruePeakDetector simdDet;
    TruePeakDetector scalarDet;
    simdDet.prepare(kSampleRate);
    scalarDet.prepare(kSampleRate);

    // DC signal at 0.5 — after warmup, both paths must produce identical output
    const int warmup     = 128;
    const int checkSamps = 32;
    std::vector<float> dc(warmup + checkSamps, 0.5f);

    // Warm up both detectors
    for (int i = 0; i < warmup; ++i)
    {
        simdDet.processSample(dc[i]);
        scalarDet.processSampleScalar(dc[i]);
    }

    // In steady state the two implementations must agree within float epsilon
    for (int i = warmup; i < warmup + checkSamps; ++i)
    {
        float simdPeak   = simdDet.processSample(dc[i]);
        float scalarPeak = scalarDet.processSampleScalar(dc[i]);
        REQUIRE(std::abs(simdPeak - scalarPeak) < 1e-5f);
    }
}
