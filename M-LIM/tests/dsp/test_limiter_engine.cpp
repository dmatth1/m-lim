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
