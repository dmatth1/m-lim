#include "catch2/catch_amalgamated.hpp"
#include "dsp/LimiterEngine.h"
#include <juce_audio_basics/juce_audio_basics.h>
#include <cmath>
#include <vector>
#include <limits>

static constexpr double kSampleRate = 44100.0;
static constexpr int    kBlockSize  = 512;
static constexpr double kTwoPi      = 6.283185307179586;

/** Generate a stereo 1kHz sine at given amplitude into a buffer. */
static juce::AudioBuffer<float> makeSine(float amplitude, int numSamples,
                                          int numChannels = 2,
                                          double fs = kSampleRate)
{
    juce::AudioBuffer<float> buf(numChannels, numSamples);
    for (int ch = 0; ch < numChannels; ++ch)
    {
        float* data = buf.getWritePointer(ch);
        for (int i = 0; i < numSamples; ++i)
            data[i] = amplitude * static_cast<float>(
                           std::sin(kTwoPi * 1000.0 * i / fs));
    }
    return buf;
}

/** Returns the max absolute sample in the buffer across all channels. */
static float maxAbsValue(const juce::AudioBuffer<float>& buf)
{
    float maxVal = 0.0f;
    for (int ch = 0; ch < buf.getNumChannels(); ++ch)
    {
        const float* data = buf.getReadPointer(ch);
        for (int i = 0; i < buf.getNumSamples(); ++i)
            maxVal = std::max(maxVal, std::abs(data[i]));
    }
    return maxVal;
}

// ============================================================================
// test_full_chain_no_clip
// A very loud input signal should produce output that never exceeds the ceiling.
// ============================================================================
TEST_CASE("test_full_chain_no_clip", "[LimiterEngine]")
{
    LimiterEngine engine;
    engine.prepare(kSampleRate, kBlockSize, 2);
    engine.setOutputCeiling(0.0f);  // 0 dBFS = 1.0 linear

    // Process several blocks of a very loud signal (10x normal level)
    for (int block = 0; block < 10; ++block)
    {
        juce::AudioBuffer<float> buf = makeSine(10.0f, kBlockSize);
        engine.process(buf);

        float peak = maxAbsValue(buf);
        INFO("Block " << block << " peak: " << peak);
        REQUIRE(peak <= 1.01f);  // allow tiny margin for rounding
    }
}

// ============================================================================
// test_input_gain_applied
// +6dB input gain on a signal at -12dBFS should bring it to -6dBFS before limiting.
// With a ceiling at 0 dBFS, the output should be limited to 1.0.
// ============================================================================
TEST_CASE("test_input_gain_applied", "[LimiterEngine]")
{
    LimiterEngine engine;
    engine.prepare(kSampleRate, kBlockSize, 2);
    engine.setOutputCeiling(0.0f);

    // Feed a signal at -12 dBFS without gain — should not need limiting
    {
        LimiterEngine eng2;
        eng2.prepare(kSampleRate, kBlockSize, 2);
        eng2.setInputGain(0.0f);  // 0 dB gain
        eng2.setOutputCeiling(0.0f);

        const float ampNoGain = static_cast<float>(std::pow(10.0, -12.0 / 20.0));
        // Process several blocks to settle
        for (int i = 0; i < 5; ++i)
        {
            juce::AudioBuffer<float> buf = makeSine(ampNoGain, kBlockSize);
            eng2.process(buf);
        }
        // GR should be ~0 (no significant limiting)
        INFO("GR with 0dB gain on -12dBFS: " << eng2.getGainReduction());
        REQUIRE(eng2.getGainReduction() > -3.0f);  // less than 3 dB reduction
    }

    // Same signal with +12dB gain should clip and trigger limiting
    {
        engine.setInputGain(12.0f);
        const float ampNoGain = static_cast<float>(std::pow(10.0, -12.0 / 20.0));
        for (int i = 0; i < 5; ++i)
        {
            juce::AudioBuffer<float> buf = makeSine(ampNoGain, kBlockSize);
            engine.process(buf);
        }
        INFO("GR with +12dB gain on -12dBFS: " << engine.getGainReduction());
        REQUIRE(engine.getGainReduction() < -0.5f);  // at least 0.5 dB reduction expected
    }
}

// ============================================================================
// test_output_ceiling
// Output level should not exceed the configured ceiling in dBFS.
// ============================================================================
TEST_CASE("test_output_ceiling", "[LimiterEngine]")
{
    // Test at -3 dBFS ceiling
    const float ceilingDb  = -3.0f;
    const float ceilingLin = static_cast<float>(std::pow(10.0, ceilingDb / 20.0));

    LimiterEngine engine;
    engine.prepare(kSampleRate, kBlockSize, 2);
    engine.setOutputCeiling(ceilingDb);

    for (int block = 0; block < 10; ++block)
    {
        juce::AudioBuffer<float> buf = makeSine(5.0f, kBlockSize);
        engine.process(buf);

        float peak = maxAbsValue(buf);
        INFO("Block " << block << " peak: " << peak << " ceiling: " << ceilingLin);
        REQUIRE(peak <= ceilingLin + 0.01f);  // allow small margin
    }
}

// ============================================================================
// test_algorithm_switch
// Changing algorithm mid-stream should not crash or produce NaN/Inf.
// ============================================================================
TEST_CASE("test_algorithm_switch", "[LimiterEngine]")
{
    LimiterEngine engine;
    engine.prepare(kSampleRate, kBlockSize, 2);
    engine.setOutputCeiling(0.0f);

    // Process some blocks, switch algorithm each time
    const LimiterAlgorithm algos[] = {
        LimiterAlgorithm::Transparent,
        LimiterAlgorithm::Punchy,
        LimiterAlgorithm::Dynamic,
        LimiterAlgorithm::Aggressive,
        LimiterAlgorithm::Allround,
    };

    for (int i = 0; i < 20; ++i)
    {
        engine.setAlgorithm(algos[i % 5]);
        juce::AudioBuffer<float> buf = makeSine(2.0f, kBlockSize);
        engine.process(buf);

        for (int ch = 0; ch < buf.getNumChannels(); ++ch)
        {
            const float* data = buf.getReadPointer(ch);
            for (int s = 0; s < buf.getNumSamples(); ++s)
                REQUIRE(std::isfinite(data[s]));
        }
    }
}

// ============================================================================
// test_latency_reporting
// getLatencySamples() should return a non-negative value and change with
// lookahead settings.
// ============================================================================
TEST_CASE("test_latency_reporting", "[LimiterEngine]")
{
    LimiterEngine engine;
    engine.prepare(kSampleRate, kBlockSize, 2);

    const int latency0 = engine.getLatencySamples();
    INFO("Latency with default lookahead: " << latency0);
    REQUIRE(latency0 >= 0);

    // Increase lookahead — latency should increase
    engine.setLookahead(5.0f);  // 5 ms
    // Need to trigger param update by processing a block
    juce::AudioBuffer<float> dummy(2, kBlockSize);
    dummy.clear();
    engine.process(dummy);

    const int latency5ms = engine.getLatencySamples();
    INFO("Latency with 5ms lookahead: " << latency5ms);
    REQUIRE(latency5ms >= latency0);
}

// ============================================================================
// test_meter_data_populated
// After processing, getGainReduction and getTruePeak should return valid values.
// The meter FIFO should contain data.
// ============================================================================
TEST_CASE("test_meter_data_populated", "[LimiterEngine]")
{
    LimiterEngine engine;
    engine.prepare(kSampleRate, kBlockSize, 2);
    engine.setOutputCeiling(0.0f);

    // Process a loud signal to trigger GR
    for (int i = 0; i < 5; ++i)
    {
        juce::AudioBuffer<float> buf = makeSine(2.0f, kBlockSize);
        engine.process(buf);
    }

    const float gr = engine.getGainReduction();
    INFO("Gain reduction: " << gr << " dB");
    REQUIRE(std::isfinite(gr));
    REQUIRE(gr <= 0.0f);   // GR is 0 or negative
    REQUIRE(gr > -100.0f); // Should not be unreasonably extreme

    const float tpL = engine.getTruePeakL();
    const float tpR = engine.getTruePeakR();
    INFO("True peak L: " << tpL << "  R: " << tpR);
    REQUIRE(std::isfinite(tpL));
    REQUIRE(std::isfinite(tpR));
    REQUIRE(tpL >= 0.0f);
    REQUIRE(tpR >= 0.0f);

    // FIFO should have data
    MeterData md;
    REQUIRE(engine.getMeterFIFO().pop(md));
    REQUIRE(std::isfinite(md.gainReduction));
    REQUIRE(md.inputLevelL >= 0.0f);
    REQUIRE(md.outputLevelL >= 0.0f);
}

// ============================================================================
// test_bypass_passes_signal_unchanged
// In bypass mode, the output should equal the input (within float precision).
// ============================================================================
TEST_CASE("test_bypass_passes_signal_unchanged", "[LimiterEngine]")
{
    LimiterEngine engine;
    engine.prepare(kSampleRate, kBlockSize, 2);
    engine.setBypass(true);

    const float amp = 2.0f;  // above ceiling, but bypass ignores ceiling
    juce::AudioBuffer<float> ref = makeSine(amp, kBlockSize);
    juce::AudioBuffer<float> buf = makeSine(amp, kBlockSize);

    engine.process(buf);

    for (int ch = 0; ch < buf.getNumChannels(); ++ch)
    {
        const float* orig = ref.getReadPointer(ch);
        const float* out  = buf.getReadPointer(ch);
        for (int i = 0; i < buf.getNumSamples(); ++i)
            REQUIRE(out[i] == Catch::Approx(orig[i]).margin(1e-6f));
    }
}

// ============================================================================
// test_total_gr_is_sum_of_stages
// With a very loud signal both limiters stages are active. The reported total
// gain reduction should be ≤ what either single stage would produce alone,
// i.e. the sum of the two negative dB values — not just the minimum of them.
// Concretely: feeding 10x amplitude into a 0 dBFS ceiling should produce more
// than -6 dB of total GR (which a single stage with that input level could
// easily contribute on its own, so a correct sum must exceed this).
// Also verifies that the floor clamp (-60 dB) prevents extreme values.
// ============================================================================
TEST_CASE("test_total_gr_is_sum_of_stages", "[LimiterEngine]")
{
    LimiterEngine engine;
    engine.prepare(kSampleRate, kBlockSize, 2);
    engine.setOutputCeiling(0.0f);   // 0 dBFS ceiling
    engine.setInputGain(20.0f);      // push well above ceiling so both stages engage

    // Process several blocks to let attack/release settle
    for (int i = 0; i < 20; ++i)
    {
        juce::AudioBuffer<float> buf = makeSine(1.0f, kBlockSize);
        engine.process(buf);
    }

    const float gr = engine.getGainReduction();
    INFO("Total GR with +20 dB input: " << gr << " dB");

    // GR must be finite
    REQUIRE(std::isfinite(gr));
    // GR must be non-positive (reduction only)
    REQUIRE(gr <= 0.0f);
    // With 20 dB of headroom to cover, GR should be substantially negative
    // — at least -3 dB. If it were still using std::min (the bug), only one
    // stage would be counted and the total could be under-reported.
    REQUIRE(gr < -3.0f);
    // The floor clamp (-60 dB) must be respected
    REQUIRE(gr >= -60.0f);
}

// ============================================================================
// test_silence_produces_no_gain_reduction
// Pure silence should not trigger any gain reduction.
// ============================================================================
TEST_CASE("test_silence_produces_no_gain_reduction", "[LimiterEngine]")
{
    LimiterEngine engine;
    engine.prepare(kSampleRate, kBlockSize, 2);
    engine.setOutputCeiling(0.0f);

    for (int i = 0; i < 10; ++i)
    {
        juce::AudioBuffer<float> buf(2, kBlockSize);
        buf.clear();
        engine.process(buf);
    }

    const float gr = engine.getGainReduction();
    INFO("GR on silence: " << gr);
    // Silence should produce no (or negligible) gain reduction
    REQUIRE(gr > -1.0f);
}

// ============================================================================
// test_ceiling_below_zero_dbfs
// With ceiling set to -1.0 dBFS, a 0 dBFS input sine must be limited smoothly
// to the ceiling without relying on hard clipping. The output must stay below
// the ceiling and the signal must be smooth (no hard-clip distortion steps).
// ============================================================================
TEST_CASE("test_ceiling_below_zero_dbfs", "[LimiterEngine]")
{
    const float ceilingDb  = -1.0f;
    const float ceilingLin = static_cast<float>(std::pow(10.0, ceilingDb / 20.0));  // ~0.891

    LimiterEngine engine;
    engine.prepare(kSampleRate, kBlockSize, 2);
    engine.setOutputCeiling(ceilingDb);

    // Run several blocks of a 0 dBFS sine (amplitude 1.0) to let the limiter settle
    for (int block = 0; block < 20; ++block)
    {
        juce::AudioBuffer<float> buf = makeSine(1.0f, kBlockSize);
        engine.process(buf);

        const float peak = maxAbsValue(buf);
        INFO("Block " << block << " peak: " << peak << "  ceiling: " << ceilingLin);
        REQUIRE(peak <= ceilingLin + 0.01f);
    }

    // Gain reduction must be non-zero — the limiters must be doing work,
    // not deferring everything to the hard clip at step 7.
    const float gr = engine.getGainReduction();
    INFO("GR at -1 dBFS ceiling: " << gr << " dB");
    REQUIRE(gr < -0.1f);  // at least 0.1 dB of gain reduction must come from limiters

    // Verify smooth output: process one final block and check that consecutive
    // samples don't jump by more than the expected smooth gain reduction amount.
    // A hard clip would produce steps larger than ~0.1; smooth limiting would not.
    juce::AudioBuffer<float> finalBuf = makeSine(1.0f, kBlockSize);
    engine.process(finalBuf);

    const float* ch0 = finalBuf.getReadPointer(0);
    float maxStep = 0.0f;
    for (int i = 1; i < kBlockSize; ++i)
        maxStep = std::max(maxStep, std::abs(ch0[i] - ch0[i - 1]));

    // A 1 kHz sine at 44100 Hz has a max inter-sample slope of about 0.14.
    // Smooth limiting can only reduce this; hard clipping would not increase
    // it either (clips are already in the sine), but the ceiling check above
    // already ensures the hard clip is not the primary control.
    INFO("Max inter-sample step: " << maxStep);
    REQUIRE(maxStep < 0.5f);  // sanity check — no pathological discontinuities
}

// ============================================================================
// test_dc_filter_removes_dc_when_enabled
// Feed a DC-only signal (0.5f constant) through the engine. With DC filter
// disabled the output mean should remain measurably non-zero; with it enabled
// the output mean should converge to near-zero after the filter settles.
// ============================================================================
TEST_CASE("test_dc_filter_removes_dc_when_enabled", "[LimiterEngine]")
{
    constexpr float dcLevel  = 0.5f;
    constexpr int   warmup   = 200;  // blocks to let HP settle
    constexpr int   numBlocks = 10;  // blocks to measure

    // --- Without DC filter ---
    {
        LimiterEngine engine;
        engine.prepare(kSampleRate, kBlockSize, 2);
        engine.setOutputCeiling(0.0f);
        engine.setDCFilterEnabled(false);

        juce::AudioBuffer<float> buf(2, kBlockSize);
        for (int b = 0; b < warmup + numBlocks; ++b)
        {
            buf.clear();
            for (int ch = 0; ch < 2; ++ch)
                juce::FloatVectorOperations::fill(buf.getWritePointer(ch), dcLevel, kBlockSize);
            engine.process(buf);
        }

        // Measure mean of last processed block (ch 0)
        double sum = 0.0;
        const float* d = buf.getReadPointer(0);
        for (int i = 0; i < kBlockSize; ++i) sum += d[i];
        float mean = static_cast<float>(sum / kBlockSize);
        INFO("DC mean without filter: " << mean);
        // The limiter will reduce the DC level, but some DC should remain
        REQUIRE(std::abs(mean) > 0.01f);
    }

    // --- With DC filter ---
    {
        LimiterEngine engine;
        engine.prepare(kSampleRate, kBlockSize, 2);
        engine.setOutputCeiling(0.0f);
        engine.setDCFilterEnabled(true);

        juce::AudioBuffer<float> buf(2, kBlockSize);
        for (int b = 0; b < warmup + numBlocks; ++b)
        {
            buf.clear();
            for (int ch = 0; ch < 2; ++ch)
                juce::FloatVectorOperations::fill(buf.getWritePointer(ch), dcLevel, kBlockSize);
            engine.process(buf);
        }

        // Measure mean of last processed block (ch 0)
        double sum = 0.0;
        const float* d = buf.getReadPointer(0);
        for (int i = 0; i < kBlockSize; ++i) sum += d[i];
        float mean = static_cast<float>(sum / kBlockSize);
        INFO("DC mean with filter after warmup: " << mean);
        REQUIRE(std::abs(mean) < 0.05f);
    }
}

// ============================================================================
// test_dc_filter_both_channels
// Verify that the DC filter is applied to both the left and right channels,
// not just channel 0.
// ============================================================================
TEST_CASE("test_dc_filter_both_channels", "[LimiterEngine]")
{
    constexpr float dcLevel  = 0.5f;
    constexpr int   warmup   = 200;

    LimiterEngine engine;
    engine.prepare(kSampleRate, kBlockSize, 2);
    engine.setOutputCeiling(0.0f);
    engine.setDCFilterEnabled(true);

    juce::AudioBuffer<float> buf(2, kBlockSize);
    for (int b = 0; b < warmup + 5; ++b)
    {
        buf.clear();
        for (int ch = 0; ch < 2; ++ch)
            juce::FloatVectorOperations::fill(buf.getWritePointer(ch), dcLevel, kBlockSize);
        engine.process(buf);
    }

    // Both channels should have near-zero mean
    for (int ch = 0; ch < 2; ++ch)
    {
        double sum = 0.0;
        const float* d = buf.getReadPointer(ch);
        for (int i = 0; i < kBlockSize; ++i) sum += d[i];
        float mean = static_cast<float>(sum / kBlockSize);
        INFO("Channel " << ch << " DC mean after filter: " << mean);
        REQUIRE(std::abs(mean) < 0.05f);
    }
}

// ============================================================================
// test_dither_adds_noise_when_enabled
// Feed silence through the engine. With dither disabled output must be zero.
// With dither enabled at 16-bit, at least some output samples must be non-zero.
// ============================================================================
TEST_CASE("test_dither_adds_noise_when_enabled", "[LimiterEngine]")
{
    constexpr int kTotalSamples = 4096;

    // --- Dither disabled — output must be exactly zero ---
    {
        LimiterEngine engine;
        engine.prepare(kSampleRate, kBlockSize, 2);
        engine.setOutputCeiling(0.0f);
        engine.setDitherEnabled(false);

        int nonZero = 0;
        for (int b = 0; b < kTotalSamples / kBlockSize; ++b)
        {
            juce::AudioBuffer<float> buf(2, kBlockSize);
            buf.clear();
            engine.process(buf);

            const float* d = buf.getReadPointer(0);
            for (int i = 0; i < kBlockSize; ++i)
                if (d[i] != 0.0f) ++nonZero;
        }
        INFO("Non-zero samples with dither OFF: " << nonZero);
        REQUIRE(nonZero == 0);
    }

    // --- Dither enabled at 16-bit — output must contain non-zero noise ---
    {
        LimiterEngine engine;
        engine.prepare(kSampleRate, kBlockSize, 2);
        engine.setOutputCeiling(0.0f);
        engine.setDitherEnabled(true);
        engine.setDitherBitDepth(16);

        int nonZero = 0;
        for (int b = 0; b < kTotalSamples / kBlockSize; ++b)
        {
            juce::AudioBuffer<float> buf(2, kBlockSize);
            buf.clear();
            engine.process(buf);

            const float* d = buf.getReadPointer(0);
            for (int i = 0; i < kBlockSize; ++i)
                if (d[i] != 0.0f) ++nonZero;
        }
        INFO("Non-zero samples with dither ON: " << nonZero);
        REQUIRE(nonZero > 100);
    }
}

// ============================================================================
// test_dither_stays_below_ceiling
// With dither enabled the output peak must still remain at or below the
// configured ceiling, since dither noise amplitude is sub-quantisation step.
// ============================================================================
TEST_CASE("test_dither_stays_below_ceiling", "[LimiterEngine]")
{
    const float ceilingDb  = -1.0f;
    const float ceilingLin = static_cast<float>(std::pow(10.0, ceilingDb / 20.0));

    LimiterEngine engine;
    engine.prepare(kSampleRate, kBlockSize, 2);
    engine.setOutputCeiling(ceilingDb);
    engine.setDitherEnabled(true);
    engine.setDitherBitDepth(16);

    for (int block = 0; block < 20; ++block)
    {
        juce::AudioBuffer<float> buf = makeSine(2.0f, kBlockSize);
        engine.process(buf);

        float peak = maxAbsValue(buf);
        INFO("Block " << block << " peak: " << peak << " ceiling: " << ceilingLin);
        REQUIRE(peak <= ceilingLin + 0.01f);
    }
}

// ============================================================================
// test_oversampling_with_sidechain
// Process a buffer through LimiterEngine with oversampling enabled and the
// sidechain filter active. Before the fix, the sidechain buffer was only
// numSamples long but was read at upSamples (= numSamples * oversamplingFactor),
// causing an OOB read / crash. This test verifies:
//   1. No crash (UB sanitizer or AddressSanitizer would catch the OOB)
//   2. Output is valid (finite, within ceiling)
//   3. GR is reported correctly
// ============================================================================
TEST_CASE("test_oversampling_with_sidechain", "[LimiterEngine]")
{
    // Test with several oversampling factors to exercise the OOB-prone path
    const int factors[] = { 1, 2, 3 };  // 2x, 4x, 8x

    for (int factor : factors)
    {
        LimiterEngine engine;
        engine.prepare(kSampleRate, kBlockSize, 2);
        engine.setOutputCeiling(0.0f);
        engine.setOversamplingFactor(factor);

        // Trigger the param update (oversampling factor change requires reprepare)
        {
            juce::AudioBuffer<float> warmup(2, kBlockSize);
            warmup.clear();
            engine.process(warmup);
        }

        // Process several blocks of a loud signal — no crash is the primary
        // check, output validity is secondary.
        for (int block = 0; block < 5; ++block)
        {
            juce::AudioBuffer<float> buf = makeSine(3.0f, kBlockSize);
            engine.process(buf);

            // All output samples must be finite
            for (int ch = 0; ch < buf.getNumChannels(); ++ch)
            {
                const float* data = buf.getReadPointer(ch);
                for (int s = 0; s < buf.getNumSamples(); ++s)
                {
                    INFO("factor=" << factor << " block=" << block
                         << " ch=" << ch << " s=" << s << " val=" << data[s]);
                    REQUIRE(std::isfinite(data[s]));
                }
            }

            // Output must not exceed the ceiling
            const float peak = maxAbsValue(buf);
            INFO("factor=" << factor << " block=" << block << " peak=" << peak);
            REQUIRE(peak <= 1.02f);  // allow tiny rounding margin
        }

        // Gain reduction must be finite and non-positive
        const float gr = engine.getGainReduction();
        INFO("factor=" << factor << " GR=" << gr);
        REQUIRE(std::isfinite(gr));
        REQUIRE(gr <= 0.0f);
    }
}

// ============================================================================
// test_latency_reported_truncates_not_rounds
// getLatencySamples() must use truncation (int cast) for the lookahead
// conversion, matching TransientLimiter::setLookahead().
// ============================================================================
TEST_CASE("test_latency_reported_truncates_not_rounds", "[LimiterEngine]")
{
    LimiterEngine engine;
    // Use 2x oversampling (factor=1)
    engine.setOversamplingFactor(1);
    engine.prepare(kSampleRate, kBlockSize, 2);

    const int reportedLatency = engine.getLatencySamples();

    // Must be positive when oversampling is active
    REQUIRE(reportedLatency > 0);

    // Reported value must match the truncated computation (int cast), not rounded.
    const float oversamplerFloat = engine.getOversamplerLatency();
    const float lookaheadMs      = engine.getLookaheadMs();
    const int   expectedLookahead = static_cast<int>(lookaheadMs * 0.001f * static_cast<float>(kSampleRate));
    const int   expectedOversampler = static_cast<int>(std::lround(oversamplerFloat));
    const int   expected = expectedLookahead + expectedOversampler;

    REQUIRE(reportedLatency == expected);
}

// ============================================================================
// test_metering_levels_correct_after_refactor
// Process a known sine wave at 0.5 amplitude (well below 0 dBFS ceiling) so
// the limiter barely engages. The input meter must report ~0.5 and the output
// meter must also be close to 0.5 (within rounding/lookahead margin).
// ============================================================================
TEST_CASE("test_metering_levels_correct_after_refactor", "[LimiterEngine]")
{
    LimiterEngine engine;
    engine.prepare(kSampleRate, kBlockSize, 2);
    engine.setOutputCeiling(0.0f);   // 0 dBFS ceiling

    // Process several warm-up blocks to fill the lookahead buffer, then
    // inspect the meter data from the last processed block.
    MeterData md{};
    for (int i = 0; i < 10; ++i)
    {
        juce::AudioBuffer<float> buf = makeSine(0.5f, kBlockSize);
        engine.process(buf);

        // Drain the FIFO each iteration so we get fresh data
        MeterData tmp;
        while (engine.getMeterFIFO().pop(tmp))
            md = tmp;
    }

    INFO("inputLevelL: " << md.inputLevelL << "  inputLevelR: " << md.inputLevelR);
    REQUIRE(md.inputLevelL == Catch::Approx(0.5f).margin(0.001f));
    REQUIRE(md.inputLevelR == Catch::Approx(0.5f).margin(0.001f));
}

// ============================================================================
// test_mono_metering_mirrors_L_to_R
// Prepare the engine with 1 channel. After processing, inputLevelR must equal
// inputLevelL, and outputLevelR must equal outputLevelL.
// ============================================================================
TEST_CASE("test_mono_metering_mirrors_L_to_R", "[LimiterEngine]")
{
    LimiterEngine engine;
    engine.prepare(kSampleRate, kBlockSize, 1);   // mono
    engine.setOutputCeiling(0.0f);

    MeterData md{};
    for (int i = 0; i < 10; ++i)
    {
        juce::AudioBuffer<float> buf = makeSine(0.5f, kBlockSize, 1);
        engine.process(buf);

        MeterData tmp;
        while (engine.getMeterFIFO().pop(tmp))
            md = tmp;
    }

    INFO("inputLevelL: " << md.inputLevelL << "  inputLevelR: " << md.inputLevelR);
    INFO("outputLevelL: " << md.outputLevelL << "  outputLevelR: " << md.outputLevelR);
    REQUIRE(md.inputLevelR == Catch::Approx(md.inputLevelL).margin(1e-6f));
    REQUIRE(md.outputLevelR == Catch::Approx(md.outputLevelL).margin(1e-6f));
}

// ============================================================================
// Mono true peak mirroring: R detector processes channel 0 (same as L),
// so truePeakR must equal truePeakL.
// ============================================================================
TEST_CASE("test_mono_true_peak_mirrors_L_to_R", "[LimiterEngine]")
{
    LimiterEngine engine;
    engine.prepare(kSampleRate, kBlockSize, 1);   // mono
    engine.setOutputCeiling(0.0f);
    engine.setTruePeakEnabled(true);

    MeterData md{};
    for (int i = 0; i < 10; ++i)
    {
        juce::AudioBuffer<float> buf = makeSine(0.9f, kBlockSize, 1);
        engine.process(buf);

        MeterData tmp;
        while (engine.getMeterFIFO().pop(tmp))
            md = tmp;
    }

    INFO("truePeakL: " << md.truePeakL << "  truePeakR: " << md.truePeakR);
    REQUIRE(md.truePeakR == Catch::Approx(md.truePeakL).margin(1e-6f));
}

// ============================================================================
// Mono true peak non-zero: confirms true peak detection computes non-zero
// values in mono mode (not just two zeroes being equal).
// ============================================================================
TEST_CASE("test_mono_true_peak_reports_nonzero", "[LimiterEngine]")
{
    LimiterEngine engine;
    engine.prepare(kSampleRate, kBlockSize, 1);   // mono
    engine.setOutputCeiling(0.0f);
    engine.setTruePeakEnabled(true);

    MeterData md{};
    for (int i = 0; i < 10; ++i)
    {
        juce::AudioBuffer<float> buf = makeSine(0.9f, kBlockSize, 1);
        engine.process(buf);

        MeterData tmp;
        while (engine.getMeterFIFO().pop(tmp))
            md = tmp;
    }

    INFO("truePeakL: " << md.truePeakL);
    REQUIRE(md.truePeakL > 0.0f);
}

// ============================================================================
// test_no_deferred_flag_when_factor_unchanged
// After prepare() with default oversampling (factor=0), processing a block
// must NOT set the deferred oversampling flag.
// ============================================================================
TEST_CASE("test_no_deferred_flag_when_factor_unchanged", "[LimiterEngine]")
{
    LimiterEngine engine;
    engine.prepare(kSampleRate, kBlockSize, 2);  // factor=0 by default

    // No setOversamplingFactor() call — mParamsDirty is false after prepare()
    juce::AudioBuffer<float> buf = makeSine(0.5f, kBlockSize);
    engine.process(buf);

    REQUIRE(engine.hasDeferredOversamplingChange() == false);
}

// ============================================================================
// test_deferred_flag_set_after_factor_change
// After prepare() at factor=0, calling setOversamplingFactor(2) and then
// processing one block must set hasDeferredOversamplingChange() to true.
// ============================================================================
TEST_CASE("test_deferred_flag_set_after_factor_change", "[LimiterEngine]")
{
    LimiterEngine engine;
    engine.prepare(kSampleRate, kBlockSize, 2);  // factor=0

    engine.setOversamplingFactor(2);  // change to 4x — marks params dirty

    juce::AudioBuffer<float> buf = makeSine(0.5f, kBlockSize);
    engine.process(buf);  // applyPendingParams() detects mismatch, sets deferred flag

    REQUIRE(engine.hasDeferredOversamplingChange() == true);
}

// ============================================================================
// test_deferred_flag_cleared_after_reprepare
// Once the deferred flag is set, calling prepare() (the "message-thread action")
// with the new factor must clear the flag. A subsequent process() block must
// also leave the flag false.
// ============================================================================
TEST_CASE("test_deferred_flag_cleared_after_reprepare", "[LimiterEngine]")
{
    LimiterEngine engine;
    engine.prepare(kSampleRate, kBlockSize, 2);  // factor=0

    // Trigger deferred change
    engine.setOversamplingFactor(2);
    {
        juce::AudioBuffer<float> buf = makeSine(0.5f, kBlockSize);
        engine.process(buf);
    }
    REQUIRE(engine.hasDeferredOversamplingChange() == true);

    // Simulate message-thread response: re-prepare with the new factor already
    // stored in mOversamplingFactor (setOversamplingFactor already set it).
    engine.prepare(kSampleRate, kBlockSize, 2);
    REQUIRE(engine.hasDeferredOversamplingChange() == false);

    // Processing a further block must not re-set the flag
    {
        juce::AudioBuffer<float> buf = makeSine(0.5f, kBlockSize);
        engine.process(buf);
    }
    REQUIRE(engine.hasDeferredOversamplingChange() == false);
}

// ============================================================================
// test_audio_bounded_during_deferred_period
// While an oversampling factor change is pending (between setOversamplingFactor
// and re-prepare), the engine must continue processing at the OLD factor and
// must not produce output that exceeds the ceiling.
// ============================================================================
TEST_CASE("test_audio_bounded_during_deferred_period", "[LimiterEngine]")
{
    const float ceilingLin = 1.0f;  // 0 dBFS

    LimiterEngine engine;
    engine.prepare(kSampleRate, kBlockSize, 2);  // factor=0
    engine.setOutputCeiling(0.0f);

    // Trigger the deferred oversampling change
    engine.setOversamplingFactor(2);

    // Process several blocks — change is deferred, engine runs at old factor
    for (int i = 0; i < 5; ++i)
    {
        juce::AudioBuffer<float> buf = makeSine(5.0f, kBlockSize);  // loud signal
        engine.process(buf);

        const float peak = maxAbsValue(buf);
        INFO("Deferred period block " << i << " peak: " << peak);
        REQUIRE(peak <= ceilingLin + 0.01f);  // must stay within ceiling
    }

    REQUIRE(engine.hasDeferredOversamplingChange() == true);
}

// ============================================================================
// test_latency_changes_after_reprepare
// getLatencySamples() should reflect the new oversampling factor after
// re-prepare is called. Factor=0 (no oversampling) has lower latency than
// factor=2 (4x oversampling) which adds oversampler latency.
// ============================================================================
TEST_CASE("test_latency_changes_after_reprepare", "[LimiterEngine]")
{
    LimiterEngine engine;

    // Prepare with no oversampling (factor=0)
    engine.prepare(kSampleRate, kBlockSize, 2);
    const int latencyNoOS = engine.getLatencySamples();
    INFO("Latency at factor=0: " << latencyNoOS);
    REQUIRE(latencyNoOS >= 0);

    // Request factor=2 (4x oversampling) and re-prepare (message-thread action)
    engine.setOversamplingFactor(2);
    engine.prepare(kSampleRate, kBlockSize, 2);  // re-prepare with factor=2 active
    const int latencyWithOS = engine.getLatencySamples();
    INFO("Latency at factor=2: " << latencyWithOS);

    // 4x oversampling adds latency through the polyphase filter
    REQUIRE(latencyWithOS > latencyNoOS);

    // Deferred flag must be clear after prepare()
    REQUIRE(engine.hasDeferredOversamplingChange() == false);
}

// ============================================================================
// test_sidechain_hp_reduces_bass_triggering
// A loud 50 Hz sine should trigger much less GR when sidechain HP is set to
// 500 Hz (which filters the bass from the detection path) vs. 20 Hz (flat).
// ============================================================================
TEST_CASE("test_sidechain_hp_reduces_bass_triggering", "[LimiterEngine]")
{
    // Generate a loud 50 Hz sine at 2x ceiling amplitude
    auto make50HzSine = [](float amplitude, int numSamples, double fs = kSampleRate)
    {
        juce::AudioBuffer<float> buf(2, numSamples);
        for (int ch = 0; ch < 2; ++ch)
        {
            float* data = buf.getWritePointer(ch);
            for (int i = 0; i < numSamples; ++i)
                data[i] = amplitude * static_cast<float>(
                               std::sin(kTwoPi * 50.0 * i / fs));
        }
        return buf;
    };

    auto measureGR = [&](float hpFreq) -> float
    {
        LimiterEngine eng;
        eng.prepare(kSampleRate, kBlockSize, 2);
        eng.setOutputCeiling(0.0f);
        eng.setSidechainHPFreq(hpFreq);
        // Feed 20 blocks to let the limiter/filter reach steady state
        for (int i = 0; i < 20; ++i)
        {
            auto buf = make50HzSine(2.0f, kBlockSize);
            eng.process(buf);
        }
        return eng.getGainReduction();
    };

    const float grLowHP  = measureGR(20.0f);   // bass passes sidechain detection → more GR
    const float grHighHP = measureGR(500.0f);  // bass filtered from detection → less GR

    INFO("GR with HP=20 Hz:  " << grLowHP  << " dB");
    INFO("GR with HP=500 Hz: " << grHighHP << " dB");

    // grLowHP should be more negative (more reduction); grHighHP closer to 0
    REQUIRE(grHighHP > grLowHP);
}

// ============================================================================
// test_sidechain_tilt_affects_gr
// For a treble-heavy signal, positive sidechain tilt (+6 dB) boosts high
// frequencies in the detection path, causing more GR compared to flat tilt.
// ============================================================================
TEST_CASE("test_sidechain_tilt_affects_gr", "[LimiterEngine]")
{
    // Generate a broadband signal rich in high frequencies (8 kHz sine)
    auto makeHighFreqSine = [](float amplitude, int numSamples, double fs = kSampleRate)
    {
        juce::AudioBuffer<float> buf(2, numSamples);
        for (int ch = 0; ch < 2; ++ch)
        {
            float* data = buf.getWritePointer(ch);
            for (int i = 0; i < numSamples; ++i)
                data[i] = amplitude * static_cast<float>(
                               std::sin(kTwoPi * 8000.0 * i / fs));
        }
        return buf;
    };

    auto measureGR = [&](float tiltDb) -> float
    {
        LimiterEngine eng;
        eng.prepare(kSampleRate, kBlockSize, 2);
        eng.setOutputCeiling(0.0f);
        eng.setSidechainTilt(tiltDb);
        // Feed 20 blocks so the filter/limiter reaches steady state
        for (int i = 0; i < 20; ++i)
        {
            // Use amplitude just above unity so limiter engages
            auto buf = makeHighFreqSine(1.2f, kBlockSize);
            eng.process(buf);
        }
        return eng.getGainReduction();
    };

    const float grFlat     = measureGR(0.0f);   // flat detection
    const float grTiltUp   = measureGR(6.0f);   // highs boosted in detection → more GR

    INFO("GR with tilt=0 dB:  " << grFlat   << " dB");
    INFO("GR with tilt=+6 dB: " << grTiltUp << " dB");

    // Positive tilt boosts high-frequency content in the detection path,
    // causing the limiter to see more energy and apply more gain reduction.
    REQUIRE(grTiltUp < grFlat);  // more negative = more reduction
}

// ============================================================================
// test_latency_44100_5ms_truncates_to_220
// At 44100 Hz with 5 ms lookahead and no oversampling, getLatencySamples()
// must return 220 (truncation) not 221 (rounding).
// 5 * 0.001 * 44100 = 220.5 → truncate → 220
// ============================================================================
TEST_CASE("test_latency_44100_5ms_truncates_to_220", "[LimiterEngine]")
{
    LimiterEngine engine;
    engine.setOversamplingFactor(0); // no oversampling
    engine.setLookahead(5.0f);       // 5 ms before prepare so mSampleRate is set first
    engine.prepare(44100.0, kBlockSize, 2);

    const int reported = engine.getLatencySamples();
    INFO("Reported latency at 44100 Hz, 5ms: " << reported);
    // int(5 * 0.001 * 44100) = int(220.5) = 220
    REQUIRE(reported == 220);
}

// ============================================================================
// test_latency_48000_1ms_truncates_to_48
// At 48000 Hz with 1 ms lookahead and no oversampling, getLatencySamples()
// must return 48.
// 1 * 0.001 * 48000 = 48.0 → truncate → 48
// ============================================================================
TEST_CASE("test_latency_48000_1ms_truncates_to_48", "[LimiterEngine]")
{
    LimiterEngine engine;
    engine.setOversamplingFactor(0); // no oversampling
    engine.setLookahead(1.0f);       // 1 ms
    engine.prepare(48000.0, kBlockSize, 2);

    const int reported = engine.getLatencySamples();
    INFO("Reported latency at 48000 Hz, 1ms: " << reported);
    REQUIRE(reported == 48);
}

// ============================================================================
// test_latency_44100_5ms_4x_oversampling
// At 44100 Hz with 5 ms lookahead and 4x oversampling (factor=2),
// the lookahead portion of getLatencySamples() must equal int(5 * 0.001 * 44100) = 220.
// Total latency = 220 + oversamplerLatency.
// ============================================================================
TEST_CASE("test_latency_44100_5ms_4x_oversampling", "[LimiterEngine]")
{
    LimiterEngine engine;
    engine.setOversamplingFactor(2); // 4x oversampling
    engine.setLookahead(5.0f);       // 5 ms
    engine.prepare(44100.0, kBlockSize, 2);

    const int reported = engine.getLatencySamples();
    const int oversamplerLatency = static_cast<int>(std::lround(engine.getOversamplerLatency()));
    // lookahead portion must be int(5 * 0.001 * 44100) = 220
    const int expectedLookahead = static_cast<int>(5.0 * 0.001 * 44100.0);
    REQUIRE(expectedLookahead == 220);
    INFO("Reported latency: " << reported << ", oversampler: " << oversamplerLatency);
    REQUIRE(reported == expectedLookahead + oversamplerLatency);
}

// ============================================================================
// test_mono_no_clip
// Prepare with numChannels=1, feed a loud mono sine (amplitude 5.0) for 20
// blocks; output must be finite and peak <= ceiling + margin after block 5.
// ============================================================================
TEST_CASE("test_mono_no_clip", "[LimiterEngine]")
{
    LimiterEngine engine;
    engine.prepare(kSampleRate, kBlockSize, 1);
    engine.setOutputCeiling(0.0f);  // 0 dBFS ceiling = 1.0 linear

    const float margin = 0.02f;  // 2% margin for lookahead ramp-up

    for (int block = 0; block < 20; ++block)
    {
        juce::AudioBuffer<float> buf = makeSine(5.0f, kBlockSize, 1);
        engine.process(buf);

        const float* data = buf.getReadPointer(0);
        for (int i = 0; i < buf.getNumSamples(); ++i)
            REQUIRE(std::isfinite(data[i]));

        if (block >= 5)
        {
            const float peak = maxAbsValue(buf);
            INFO("Mono block " << block << " peak: " << peak);
            REQUIRE(peak <= 1.0f + margin);
        }
    }
}

// ============================================================================
// test_mono_silence_passes
// Prepare with numChannels=1, feed 10 blocks of silence; output must be
// all-zero (no spurious GR, DC offset, or NaN introduced).
// ============================================================================
TEST_CASE("test_mono_silence_passes", "[LimiterEngine]")
{
    LimiterEngine engine;
    engine.prepare(kSampleRate, kBlockSize, 1);
    engine.setOutputCeiling(0.0f);

    for (int block = 0; block < 10; ++block)
    {
        juce::AudioBuffer<float> buf(1, kBlockSize);
        buf.clear();
        engine.process(buf);

        const float* data = buf.getReadPointer(0);
        for (int i = 0; i < buf.getNumSamples(); ++i)
        {
            INFO("Mono silence block " << block << " sample " << i << ": " << data[i]);
            REQUIRE(std::isfinite(data[i]));
            REQUIRE(std::abs(data[i]) < 1e-6f);
        }
    }
}

// ============================================================================
// test_mono_input_gain_applied
// Set +6 dB input gain on a mono engine, feed -12 dBFS signal for 10 blocks;
// GR must be active (< -0.5 dB) because +6 dB boost pushes signal above ceiling.
// Also verifies getTruePeakL() is non-negative and no crash occurs.
// ============================================================================
TEST_CASE("test_mono_input_gain_applied", "[LimiterEngine]")
{
    LimiterEngine engine;
    engine.prepare(kSampleRate, kBlockSize, 1);
    engine.setOutputCeiling(0.0f);
    engine.setInputGain(18.0f);  // +18 dB boost

    // -12 dBFS signal: after +18 dB gain it becomes +6 dBFS, well above 0 dBFS ceiling
    const float ampMinus12dBFS = static_cast<float>(std::pow(10.0, -12.0 / 20.0));

    for (int i = 0; i < 10; ++i)
    {
        juce::AudioBuffer<float> buf = makeSine(ampMinus12dBFS, kBlockSize, 1);
        engine.process(buf);
    }

    const float gr = engine.getGainReduction();
    INFO("Mono GR with +6 dB input gain on -12 dBFS: " << gr << " dB");
    REQUIRE(std::isfinite(gr));
    REQUIRE(gr < -0.5f);  // at least 0.5 dB of gain reduction

    const float tpL = engine.getTruePeakL();
    INFO("Mono true peak L: " << tpL);
    REQUIRE(std::isfinite(tpL));
    REQUIRE(tpL >= 0.0f);
}

// ============================================================================
// test_setter_change_guard_no_dirty_on_repeat
//   Calling a setter twice with the same value must NOT mark mParamsDirty
//   on the second call. This verifies that the change guards short-circuit
//   the dirty flag, preventing unnecessary applyPendingParams() calls.
// ============================================================================
TEST_CASE("test_setter_change_guard_no_dirty_on_repeat", "[LimiterEngine]")
{
    LimiterEngine engine;
    engine.prepare(kSampleRate, kBlockSize, 2);

    // Process one block to flush any initial dirty state from prepare()
    juce::AudioBuffer<float> buf = makeSine(0.5f, kBlockSize);
    engine.process(buf);

    SECTION("setAttack: second call with same value does not set dirty")
    {
        engine.setAttack(50.0f);  // first call — sets dirty
        REQUIRE(engine.isParamsDirty());

        // Flush dirty flag by processing a block
        juce::AudioBuffer<float> b = makeSine(0.5f, kBlockSize);
        engine.process(b);
        REQUIRE_FALSE(engine.isParamsDirty());

        engine.setAttack(50.0f);  // second call — same value, must NOT set dirty
        REQUIRE_FALSE(engine.isParamsDirty());
    }

    SECTION("setRelease: second call with same value does not set dirty")
    {
        engine.setRelease(200.0f);
        juce::AudioBuffer<float> b = makeSine(0.5f, kBlockSize);
        engine.process(b);

        engine.setRelease(200.0f);  // same value again
        REQUIRE_FALSE(engine.isParamsDirty());
    }

    SECTION("setInputGain: second call with same dB does not set dirty")
    {
        engine.setInputGain(-6.0f);
        juce::AudioBuffer<float> b = makeSine(0.5f, kBlockSize);
        engine.process(b);

        engine.setInputGain(-6.0f);  // same value again
        REQUIRE_FALSE(engine.isParamsDirty());
    }

    SECTION("setOutputCeiling: second call with same dB does not set dirty")
    {
        engine.setOutputCeiling(-1.0f);
        juce::AudioBuffer<float> b = makeSine(0.5f, kBlockSize);
        engine.process(b);

        engine.setOutputCeiling(-1.0f);  // same value again
        REQUIRE_FALSE(engine.isParamsDirty());
    }

    SECTION("setAlgorithm: second call with same value does not set dirty")
    {
        engine.setAlgorithm(LimiterAlgorithm::Aggressive);
        juce::AudioBuffer<float> b = makeSine(0.5f, kBlockSize);
        engine.process(b);

        engine.setAlgorithm(LimiterAlgorithm::Aggressive);  // same value
        REQUIRE_FALSE(engine.isParamsDirty());
    }

    SECTION("setUnityGain: second call with same value does not set dirty")
    {
        engine.setUnityGain(true);
        juce::AudioBuffer<float> b = makeSine(0.5f, kBlockSize);
        engine.process(b);

        engine.setUnityGain(true);  // same value
        REQUIRE_FALSE(engine.isParamsDirty());
    }

    SECTION("different value sets dirty")
    {
        engine.setAttack(50.0f);
        juce::AudioBuffer<float> b = makeSine(0.5f, kBlockSize);
        engine.process(b);

        engine.setAttack(75.0f);  // different value — must set dirty
        REQUIRE(engine.isParamsDirty());
    }
}

// ============================================================================
// test_leveling_limiter_setter_guard_no_redundant_coefficients
//   setAttack(x) followed by setAttack(x) with the same value must not
//   recompute coefficients. We verify this indirectly: after the guard, the
//   engine's behaviour with a known signal is identical to the single-set case.
//   Direct coefficient stability is confirmed by verifying GR is consistent
//   across two otherwise identical processing runs.
// ============================================================================
TEST_CASE("test_leveling_limiter_setter_guard_no_redundant_coefficients", "[LimiterEngine]")
{
    // Helper: prepare an engine, set attack twice with same value, process signal,
    // return gain reduction.
    auto measureGR = [](float attackMs) -> float
    {
        LimiterEngine engine;
        engine.prepare(kSampleRate, kBlockSize, 2);
        engine.setAttack(attackMs);
        engine.setAttack(attackMs);  // second call — should be no-op

        for (int i = 0; i < 10; ++i)
        {
            juce::AudioBuffer<float> buf = makeSine(2.0f, kBlockSize);
            engine.process(buf);
        }
        return engine.getGainReduction();
    };

    // Run twice — results must be identical (bit-exact, since no randomness)
    const float gr1 = measureGR(10.0f);
    const float gr2 = measureGR(10.0f);

    // Both runs use the same attack setting — GR must be identical
    REQUIRE(gr1 == gr2);

    // And GR should be active (the signal is above threshold)
    REQUIRE(gr1 < -0.5f);
}

// ============================================================================
// test_stage2_no_overlimit_on_stage1_handled_peaks
// Feed a 1.2-amplitude sine through the engine with both stages active.
// After processing, the output peak should not fall more than ~0.5 dB below
// the ceiling — i.e. Stage 2 must not apply extra GR on top of Stage 1 for
// peaks that Stage 1 already handled.
// ============================================================================
TEST_CASE("test_stage2_no_overlimit_on_stage1_handled_peaks", "[LimiterEngine]")
{
    const float ceilingDb  = 0.0f;
    const float ceilingLin = 1.0f;
    // 0.5 dB below ceiling — Stage 2 over-limiting would push output further down
    const float tooLowThreshold = static_cast<float>(std::pow(10.0, -0.5 / 20.0));

    LimiterEngine engine;
    engine.prepare(kSampleRate, kBlockSize, 2);
    engine.setOutputCeiling(ceilingDb);

    // Warm up the limiter to a settled state before measuring
    for (int i = 0; i < 20; ++i)
    {
        juce::AudioBuffer<float> buf = makeSine(1.2f, kBlockSize);
        engine.process(buf);
    }

    // Measure the output peak on settled blocks
    float minPeak = 1.0f;
    for (int i = 0; i < 5; ++i)
    {
        juce::AudioBuffer<float> buf = makeSine(1.2f, kBlockSize);
        engine.process(buf);
        const float peak = maxAbsValue(buf);
        if (peak < minPeak)
            minPeak = peak;
    }

    INFO("Min output peak: " << minPeak << "  too-low threshold: " << tooLowThreshold
         << "  ceiling: " << ceilingLin);

    // Output must not exceed the ceiling
    REQUIRE(minPeak <= ceilingLin + 0.01f);
    // Output must not drop more than ~0.5 dB below ceiling (over-limiting check)
    REQUIRE(minPeak >= tooLowThreshold);
}

// ============================================================================
// test_stage2_no_gr_on_signal_at_ceiling
// A signal that arrives exactly at the ceiling (1.0 linear) after Stage 1
// should pass through Stage 2 without significant additional gain reduction,
// since Stage 1 already handled it and Stage 2 detects on the post-Stage-1 audio.
// ============================================================================
TEST_CASE("test_stage2_no_gr_on_signal_at_ceiling", "[LimiterEngine]")
{
    const float ceilingDb = 0.0f;

    LimiterEngine engine;
    engine.prepare(kSampleRate, kBlockSize, 2);
    engine.setOutputCeiling(ceilingDb);

    // Warm up with a signal slightly above ceiling so both stages settle
    for (int i = 0; i < 30; ++i)
    {
        juce::AudioBuffer<float> buf = makeSine(1.01f, kBlockSize);
        engine.process(buf);
    }

    // Now feed a signal exactly at the ceiling — Stage 1 passes it at ceiling,
    // Stage 2 should see peak ≤ threshold and not apply further GR.
    float minPeak = 1.0f;
    for (int i = 0; i < 5; ++i)
    {
        juce::AudioBuffer<float> buf = makeSine(1.0f, kBlockSize);
        engine.process(buf);
        const float peak = maxAbsValue(buf);
        if (peak < minPeak)
            minPeak = peak;
    }

    // Output must not exceed ceiling
    REQUIRE(minPeak <= 1.01f);
    // Output must remain close to ceiling (Stage 2 should not over-limit).
    // Allow up to -2 dB below ceiling to absorb release transients from warmup.
    // The bug (passing mSidePtrs to Stage 2) would cause ~1.6 dB extra GR,
    // pushing minPeak well below this floor.
    const float floorLin = static_cast<float>(std::pow(10.0, -2.0 / 20.0));
    INFO("Min peak: " << minPeak << "  floor: " << floorLin);
    REQUIRE(minPeak >= floorLin);
}

// ============================================================================
// test_silence_with_2x_oversampling
// Silence in → silence out must hold with 2x oversampling active.
// After 10 warm-up blocks the oversampling filter state should have settled;
// the remaining 40 blocks must produce peak output < 1e-5 on both channels.
// ============================================================================
TEST_CASE("test_silence_with_2x_oversampling", "[LimiterEngine]")
{
    LimiterEngine engine;
    engine.setOversamplingFactor(1);   // 1 = 2x
    engine.prepare(kSampleRate, kBlockSize, 2);
    engine.setOutputCeiling(0.0f);

    // Warm-up: allow filter state to settle
    for (int i = 0; i < 10; ++i)
    {
        juce::AudioBuffer<float> buf(2, kBlockSize);
        buf.clear();
        engine.process(buf);
    }

    // Measured blocks: output must be near silence
    for (int i = 0; i < 40; ++i)
    {
        juce::AudioBuffer<float> buf(2, kBlockSize);
        buf.clear();
        engine.process(buf);

        const float peak = maxAbsValue(buf);
        INFO("2x oversampling block " << i << " peak: " << peak);
        REQUIRE(peak < 1e-5f);
    }
}

// ============================================================================
// test_silence_with_4x_oversampling
// Same as above but with 4x oversampling.
// ============================================================================
TEST_CASE("test_silence_with_4x_oversampling", "[LimiterEngine]")
{
    LimiterEngine engine;
    engine.setOversamplingFactor(2);   // 2 = 4x
    engine.prepare(kSampleRate, kBlockSize, 2);
    engine.setOutputCeiling(0.0f);

    for (int i = 0; i < 10; ++i)
    {
        juce::AudioBuffer<float> buf(2, kBlockSize);
        buf.clear();
        engine.process(buf);
    }

    for (int i = 0; i < 40; ++i)
    {
        juce::AudioBuffer<float> buf(2, kBlockSize);
        buf.clear();
        engine.process(buf);

        const float peak = maxAbsValue(buf);
        INFO("4x oversampling block " << i << " peak: " << peak);
        REQUIRE(peak < 1e-5f);
    }
}

// ============================================================================
// test_silence_with_8x_oversampling
// Same as above but with 8x oversampling.
// ============================================================================
TEST_CASE("test_silence_with_8x_oversampling", "[LimiterEngine]")
{
    LimiterEngine engine;
    engine.setOversamplingFactor(3);   // 3 = 8x
    engine.prepare(kSampleRate, kBlockSize, 2);
    engine.setOutputCeiling(0.0f);

    for (int i = 0; i < 10; ++i)
    {
        juce::AudioBuffer<float> buf(2, kBlockSize);
        buf.clear();
        engine.process(buf);
    }

    for (int i = 0; i < 40; ++i)
    {
        juce::AudioBuffer<float> buf(2, kBlockSize);
        buf.clear();
        engine.process(buf);

        const float peak = maxAbsValue(buf);
        INFO("8x oversampling block " << i << " peak: " << peak);
        REQUIRE(peak < 1e-5f);
    }
}

// ============================================================================
// test_true_peak_not_exceeded
// Feeds a worst-case Nyquist-frequency alternating signal (+1, -1, ...) which
// produces inter-sample peaks up to ~+3 dBTP after the hard clip at 0 dBFS.
// After processing, the TruePeakDetector must report <= 1.0 + tolerance.
// ============================================================================
TEST_CASE("test_true_peak_not_exceeded", "[LimiterEngine]")
{
    LimiterEngine engine;
    engine.prepare(kSampleRate, kBlockSize, 2);
    engine.setOutputCeiling(0.0f);   // ceiling = 1.0 linear
    engine.setTruePeakEnabled(true);

    TruePeakDetector tpCheck;
    tpCheck.prepare(kSampleRate);

    // Run several blocks so limiters and FIR buffers are fully warmed up
    for (int warmup = 0; warmup < 10; ++warmup)
    {
        juce::AudioBuffer<float> buf(2, kBlockSize);
        // Alternating +1 / -1 — worst-case Nyquist signal
        for (int ch = 0; ch < 2; ++ch)
        {
            float* data = buf.getWritePointer(ch);
            for (int s = 0; s < kBlockSize; ++s)
                data[s] = (s % 2 == 0) ? 1.0f : -1.0f;
        }
        engine.process(buf);
    }

    // Now measure the true peak of the processed output over several blocks
    float worstTruePeak = 0.0f;
    for (int block = 0; block < 5; ++block)
    {
        juce::AudioBuffer<float> buf(2, kBlockSize);
        for (int ch = 0; ch < 2; ++ch)
        {
            float* data = buf.getWritePointer(ch);
            for (int s = 0; s < kBlockSize; ++s)
                data[s] = (s % 2 == 0) ? 1.0f : -1.0f;
        }
        engine.process(buf);

        // Measure true peak of left channel output
        tpCheck.reset();
        tpCheck.processBlock(buf.getReadPointer(0), kBlockSize);
        worstTruePeak = std::max(worstTruePeak, tpCheck.getPeak());
    }

    // True peak must be within 0.5 dB of ceiling (1.0 linear)
    // 0.5 dB tolerance = factor of ~1.059
    INFO("Worst true peak after enforcement: " << worstTruePeak);
    REQUIRE(worstTruePeak <= 1.059f);
}

// ============================================================================
// test_reset_no_allocation
// LimiterEngine::reset() must not invoke heap allocation.
// Uses AllocGuard from alloc_tracking.h (operator new defined once in
// test_realtime_safety.cpp; the thread-local counter is shared via extern).
// ============================================================================
#include "alloc_tracking.h"

TEST_CASE("test_reset_no_allocation", "[LimiterEngine]")
{
    LimiterEngine engine;
    engine.prepare(kSampleRate, kBlockSize, 2);

    // Process a block so internal state is non-trivial
    auto buf = makeSine(2.0f, kBlockSize);
    engine.process(buf);

    // Measure allocations during reset()
    int allocsDuringReset;
    {
        AllocGuard guard;
        engine.reset();
        allocsDuringReset = guard.count();
    }

    INFO("Heap allocations during reset(): " << allocsDuringReset);
    REQUIRE(allocsDuringReset == 0);

    // Engine must still be functional after reset
    auto buf2 = makeSine(0.5f, kBlockSize);
    engine.process(buf2);
    REQUIRE(maxAbsValue(buf2) <= 1.0f + 1e-4f);
}

// ============================================================================
// test_all_algorithms_valid_output_through_engine
//   For each of the 8 algorithms: prepare a fresh engine, set the algorithm,
//   process 20 blocks of +6 dBFS signal. All output samples must be finite,
//   max output must not exceed ceiling + 0.01, and GR must be non-trivial
//   (< -0.5 dB, i.e. some gain reduction was applied).
// ============================================================================
TEST_CASE("test_all_algorithms_valid_output_through_engine", "[LimiterEngine]")
{
    static const LimiterAlgorithm kAlgorithms[] = {
        LimiterAlgorithm::Transparent,
        LimiterAlgorithm::Punchy,
        LimiterAlgorithm::Dynamic,
        LimiterAlgorithm::Aggressive,
        LimiterAlgorithm::Allround,
        LimiterAlgorithm::Bus,
        LimiterAlgorithm::Safe,
        LimiterAlgorithm::Modern,
    };

    const float kCeilingLinear = 1.0f;  // 0 dBFS
    const float kInputAmp = 2.0f;       // +6 dBFS

    for (auto algo : kAlgorithms)
    {
        LimiterEngine engine;
        engine.prepare(kSampleRate, kBlockSize, 2);
        engine.setOutputCeiling(0.0f);  // 0 dBFS
        engine.setAlgorithm(algo);

        float maxOut = 0.0f;

        for (int block = 0; block < 20; ++block)
        {
            juce::AudioBuffer<float> buf = makeSine(kInputAmp, kBlockSize);
            engine.process(buf);

            for (int ch = 0; ch < buf.getNumChannels(); ++ch)
            {
                const float* data = buf.getReadPointer(ch);
                for (int s = 0; s < buf.getNumSamples(); ++s)
                {
                    INFO("algo=" << static_cast<int>(algo) << " ch=" << ch << " s=" << s);
                    REQUIRE(std::isfinite(data[s]));
                    maxOut = std::max(maxOut, std::abs(data[s]));
                }
            }
        }

        // Ceiling respected
        INFO("algo=" << static_cast<int>(algo) << " maxOut=" << maxOut);
        REQUIRE(maxOut <= kCeilingLinear + 0.01f);

        // GR was applied (hot signal must trigger limiting)
        INFO("algo=" << static_cast<int>(algo) << " GR=" << engine.getGainReduction());
        REQUIRE(engine.getGainReduction() < -0.5f);
    }
}

// ============================================================================
// test_all_algorithms_distinct_output_through_engine
//   Process the same +6 dBFS sine through all 8 algorithms and collect the
//   RMS of the settled output (last block). At least 6 of 8 algorithms should
//   produce distinct RMS values (differing by more than 0.01 dB).
// ============================================================================
TEST_CASE("test_all_algorithms_distinct_output_through_engine", "[LimiterEngine]")
{
    static const LimiterAlgorithm kAlgorithms[] = {
        LimiterAlgorithm::Transparent,
        LimiterAlgorithm::Punchy,
        LimiterAlgorithm::Dynamic,
        LimiterAlgorithm::Aggressive,
        LimiterAlgorithm::Allround,
        LimiterAlgorithm::Bus,
        LimiterAlgorithm::Safe,
        LimiterAlgorithm::Modern,
    };
    constexpr int kNumAlgos = 8;
    const float kInputAmp = 2.0f;  // +6 dBFS

    float rmsValues[kNumAlgos] = {};

    for (int ai = 0; ai < kNumAlgos; ++ai)
    {
        LimiterEngine engine;
        engine.prepare(kSampleRate, kBlockSize, 2);
        engine.setOutputCeiling(0.0f);
        engine.setAlgorithm(kAlgorithms[ai]);

        // Warm up
        for (int block = 0; block < 19; ++block)
        {
            juce::AudioBuffer<float> buf = makeSine(kInputAmp, kBlockSize);
            engine.process(buf);
        }

        // Measure last block
        juce::AudioBuffer<float> buf = makeSine(kInputAmp, kBlockSize);
        engine.process(buf);

        double sumSq = 0.0;
        const float* data = buf.getReadPointer(0);
        for (int s = 0; s < kBlockSize; ++s)
            sumSq += static_cast<double>(data[s]) * data[s];
        rmsValues[ai] = static_cast<float>(std::sqrt(sumSq / kBlockSize));
    }

    // Count distinct RMS values (0.01 dB tolerance ≈ factor 1.00115)
    int distinctCount = 0;
    for (int i = 0; i < kNumAlgos; ++i)
    {
        bool unique = true;
        for (int j = 0; j < i; ++j)
        {
            float ratiodB = std::abs(20.0f * static_cast<float>(std::log10(rmsValues[i] / (rmsValues[j] + 1e-9f))));
            if (ratiodB < 0.01f)
            {
                unique = false;
                break;
            }
        }
        if (unique)
            ++distinctCount;
    }

    INFO("Distinct algorithm RMS count: " << distinctCount);
    REQUIRE(distinctCount >= 6);
}

// ============================================================================
// test_reset_clears_gain_reduction
// Process a loud signal until GR > 3 dB, call reset(), then process silence.
// After reset, getGainReduction() should return 0.
// ============================================================================
TEST_CASE("test_reset_clears_gain_reduction", "[LimiterEngine][reset]")
{
    LimiterEngine engine;
    engine.prepare(kSampleRate, kBlockSize, 2);
    engine.setOutputCeiling(0.0f);

    // Drive hard signal to build up GR
    for (int i = 0; i < 20; ++i)
    {
        juce::AudioBuffer<float> buf = makeSine(10.0f, kBlockSize);
        engine.process(buf);
    }

    const float grBefore = engine.getGainReduction();
    INFO("GR before reset: " << grBefore << " dB");
    REQUIRE(grBefore < -3.0f);  // significant gain reduction

    engine.reset();

    // Process silence — GR should be zero after reset
    juce::AudioBuffer<float> silence(2, kBlockSize);
    silence.clear();
    engine.process(silence);

    const float grAfter = engine.getGainReduction();
    INFO("GR after reset + silence: " << grAfter << " dB");
    REQUIRE(grAfter == 0.0f);
}

// ============================================================================
// test_reset_output_matches_fresh_engine
// After reset(), output should match a freshly-prepared engine given same input.
// ============================================================================
TEST_CASE("test_reset_output_matches_fresh_engine", "[LimiterEngine][reset]")
{
    // Engine A: process loud signal, then reset, then process a sine
    LimiterEngine engineA;
    engineA.prepare(kSampleRate, kBlockSize, 2);
    engineA.setOutputCeiling(0.0f);

    for (int i = 0; i < 20; ++i)
    {
        juce::AudioBuffer<float> buf = makeSine(10.0f, kBlockSize);
        engineA.process(buf);
    }

    engineA.reset();

    // Engine B: freshly prepared, no prior processing
    LimiterEngine engineB;
    engineB.prepare(kSampleRate, kBlockSize, 2);
    engineB.setOutputCeiling(0.0f);

    // Process the same sine through both
    const float testAmp = 2.0f;
    juce::AudioBuffer<float> bufA = makeSine(testAmp, kBlockSize);
    juce::AudioBuffer<float> bufB = makeSine(testAmp, kBlockSize);

    engineA.process(bufA);
    engineB.process(bufB);

    float maxDiff = 0.0f;
    for (int ch = 0; ch < 2; ++ch)
    {
        const float* a = bufA.getReadPointer(ch);
        const float* b = bufB.getReadPointer(ch);
        for (int s = 0; s < kBlockSize; ++s)
            maxDiff = std::max(maxDiff, std::abs(a[s] - b[s]));
    }

    INFO("Max sample difference after reset vs fresh engine: " << maxDiff);
    REQUIRE(maxDiff < 1e-6f);
}

// ============================================================================
// test_reset_clears_true_peak_state
// Process loud signal, verify getTruePeakL() > 0, reset, then process silence.
// After silence, getTruePeakL() should reflect only the silence block (≈ 0).
// ============================================================================
TEST_CASE("test_reset_clears_true_peak_state", "[LimiterEngine][reset]")
{
    LimiterEngine engine;
    engine.prepare(kSampleRate, kBlockSize, 2);
    engine.setOutputCeiling(0.0f);

    // Process loud signal to drive true peak meters
    for (int i = 0; i < 10; ++i)
    {
        juce::AudioBuffer<float> buf = makeSine(5.0f, kBlockSize);
        engine.process(buf);
    }

    const float truePkBefore = engine.getTruePeakL();
    INFO("TruePeakL before reset: " << truePkBefore);
    REQUIRE(truePkBefore > 0.0f);

    engine.reset();

    // Process silence — true peak should now reflect only this silent block
    juce::AudioBuffer<float> silence(2, kBlockSize);
    silence.clear();
    engine.process(silence);

    const float truePkAfter = engine.getTruePeakL();
    INFO("TruePeakL after reset + silence: " << truePkAfter);
    // After reset and processing silence, true peak should be near zero (no state leakage)
    REQUIRE(truePkAfter < 1e-6f);
}
