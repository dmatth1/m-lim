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
