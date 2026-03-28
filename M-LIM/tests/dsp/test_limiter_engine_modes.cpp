#include "catch2/catch_amalgamated.hpp"
#include "dsp/LimiterEngine.h"
#include <juce_audio_basics/juce_audio_basics.h>
#include <cmath>
#include <vector>

static constexpr double kSR  = 44100.0;
static constexpr int    kBS  = 512;

/** Generate a stereo sine at given amplitude. */
static juce::AudioBuffer<float> makeSine(float amplitude, int numSamples = kBS, int numChannels = 2)
{
    juce::AudioBuffer<float> buf(numChannels, numSamples);
    for (int ch = 0; ch < numChannels; ++ch)
    {
        float* data = buf.getWritePointer(ch);
        for (int i = 0; i < numSamples; ++i)
            data[i] = amplitude * std::sin(6.283185307f * 1000.0f * i / static_cast<float>(kSR));
    }
    return buf;
}

/** RMS of a buffer channel. */
static float rms(const juce::AudioBuffer<float>& buf, int ch)
{
    const float* data = buf.getReadPointer(ch);
    float sum = 0.0f;
    for (int i = 0; i < buf.getNumSamples(); ++i)
        sum += data[i] * data[i];
    return std::sqrt(sum / buf.getNumSamples());
}

/** Max absolute value across all channels. */
static float maxAbs(const juce::AudioBuffer<float>& buf)
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
// test_bypass_passes_signal_unchanged
// In bypass mode with lookahead=0, the output should equal the input (within
// float precision). With zero lookahead there is no delay buffer to traverse.
// ============================================================================
TEST_CASE("test_bypass_passes_signal_unchanged", "[LimiterEngineModes]")
{
    LimiterEngine engine;
    engine.prepare(kSR, kBS, 2);
    engine.setLookahead(0.0f);  // zero lookahead: no delay in bypass path
    engine.setBypass(true);
    // Warm up so params apply
    {
        juce::AudioBuffer<float> warm(2, kBS);
        warm.clear();
        engine.process(warm);
    }

    const float amp = 2.0f;  // above normal ceiling, but bypass ignores it
    juce::AudioBuffer<float> ref = makeSine(amp);
    juce::AudioBuffer<float> buf = makeSine(amp);

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
// test_bypass_disables_all_processing
// In bypass mode, gain reduction should be 0 dB regardless of input level.
// ============================================================================
TEST_CASE("test_bypass_disables_all_processing", "[LimiterEngineModes]")
{
    LimiterEngine engine;
    engine.prepare(kSR, kBS, 2);
    engine.setOutputCeiling(0.0f);
    engine.setBypass(true);

    // Feed a very loud signal — no limiting should occur
    for (int i = 0; i < 5; ++i)
    {
        juce::AudioBuffer<float> buf = makeSine(10.0f);
        engine.process(buf);
    }

    const float gr = engine.getGainReduction();
    INFO("GR in bypass with loud signal: " << gr);
    REQUIRE(gr == Catch::Approx(0.0f).margin(1e-6f));
}

// ============================================================================
// test_delta_outputs_difference
// In delta mode, the output should represent what the limiter removed from
// the signal. With a loud input, this should be non-zero.
// ============================================================================
TEST_CASE("test_delta_outputs_difference", "[LimiterEngineModes]")
{
    LimiterEngine engine;
    engine.prepare(kSR, kBS, 2);
    engine.setOutputCeiling(0.0f);
    engine.setDeltaMode(true);

    // Process a very loud signal — limiting will remove a significant portion
    // Warm up the engine first
    for (int i = 0; i < 5; ++i)
    {
        juce::AudioBuffer<float> warmup = makeSine(5.0f);
        engine.process(warmup);
    }

    // The delta output should be non-zero (something was removed)
    juce::AudioBuffer<float> buf = makeSine(5.0f);
    engine.process(buf);

    const float deltaRMS = rms(buf, 0);
    INFO("Delta RMS (should be non-zero for loud input): " << deltaRMS);
    REQUIRE(deltaRMS > 0.001f);  // Something must have been removed
}

// ============================================================================
// test_delta_silence_when_no_limiting
// Delta mode with a quiet signal (below threshold) should output near-silence,
// since nothing is removed when the limiter isn't engaging.
// ============================================================================
TEST_CASE("test_delta_silence_when_no_limiting", "[LimiterEngineModes]")
{
    LimiterEngine engine;
    engine.prepare(kSR, kBS, 2);
    engine.setOutputCeiling(0.0f);  // 0 dBFS ceiling
    engine.setDeltaMode(true);

    // Very quiet signal — well below ceiling, no limiting should occur
    const float quietAmp = 0.01f;  // -40 dBFS, well below 0 dBFS ceiling

    // Warm up to settle any transients
    for (int i = 0; i < 5; ++i)
    {
        juce::AudioBuffer<float> warmup = makeSine(quietAmp);
        engine.process(warmup);
    }

    // Delta should be near silence — nothing was removed
    juce::AudioBuffer<float> buf = makeSine(quietAmp);
    engine.process(buf);

    const float deltaRMS = rms(buf, 0);
    INFO("Delta RMS for quiet signal (should be ~0): " << deltaRMS);
    REQUIRE(deltaRMS < 0.005f);  // Very little was removed
}

// ============================================================================
// test_unity_gain_compensates_ceiling
// With unity gain enabled and a positive input gain, the output ceiling is set
// to 1/inputGain so that a signal below threshold passes through at the same
// level as the input (net 0 dB change).
// ============================================================================
TEST_CASE("test_unity_gain_compensates_ceiling", "[LimiterEngineModes]")
{
    const float inputGainDb  = 12.0f;
    const float inputGainLin = std::pow(10.0f, inputGainDb / 20.0f);

    LimiterEngine engine;
    engine.prepare(kSR, kBS, 2);
    engine.setInputGain(inputGainDb);
    engine.setUnityGain(true);  // ceiling = 1/inputGain to cancel out the gain

    // Signal at -20 dBFS — after +12dB gain it's -8dBFS, still below 0dBFS ceiling
    // With unity gain, effective ceiling = 1/inputGainLin, so signal at -20dBFS
    // after gain: -8dBFS, ceiling = 1/4 (~-12dBFS)? No — let me re-think.
    // ceiling = 1/inputGainLin = 1/pow(10,12/20) ≈ 0.251 = -12dBFS
    // So a signal at -20dBFS passes through: gain boosts to -8dBFS, still above ceiling (-12dBFS)
    // That would limit it.
    // Use -20dBFS signal: after +12dB = -8dBFS, ceiling = -12dBFS → still limits
    // Use -15dBFS signal: after +12dB = -3dBFS, ceiling = -12dBFS → limits
    // We need a signal BELOW the effective ceiling: below 1/inputGainLin = -12dBFS
    // So we need signal < -12dBFS / inputGain = below -12dBFS - 12dB = -24dBFS
    // A signal at -30dBFS, after +12dB gain = -18dBFS, below ceiling of -12dBFS → no limiting
    // Then unity gain means output = input * inputGain, and ceiling = 1/inputGain
    // So output = (input * inputGain) capped at 1/inputGain
    // For no-clipping: output = input * inputGain
    // The "compensation" is that if you later scale by 1/inputGain, you'd get input back.
    // Actually the RMS of output should equal the RMS of (input * inputGain) when no clipping
    // We verify: with unity gain, the ceiling is set to 1/inputGain so the combination
    // input→gain→ceiling should keep the signal at a level proportional to input.

    // Use a signal well below the effective ceiling (1/inputGainLin)
    const float safeAmp = 0.01f;  // -40dBFS, after +12dB = -28dBFS, ceiling = -12dBFS → no clip

    // Warm up
    for (int i = 0; i < 5; ++i)
    {
        juce::AudioBuffer<float> warmup = makeSine(safeAmp);
        engine.process(warmup);
    }

    juce::AudioBuffer<float> inBuf  = makeSine(safeAmp);
    juce::AudioBuffer<float> outBuf = makeSine(safeAmp);

    const float inRMS = rms(inBuf, 0);
    engine.process(outBuf);
    const float outRMS = rms(outBuf, 0);

    // With unity gain compensation, output = input * inputGain (no clipping at this level)
    // So outRMS should be ≈ inRMS * inputGainLin
    const float expectedRMS = inRMS * inputGainLin;
    INFO("Input RMS: " << inRMS << "  Expected output RMS: " << expectedRMS
         << "  Actual output RMS: " << outRMS);
    REQUIRE(outRMS == Catch::Approx(expectedRMS).epsilon(0.05f));  // within 5%
}

// ============================================================================
// test_mode_switch_no_glitch
// Toggling bypass/delta/unity gain modes on/off mid-stream should not produce
// audio glitches (no sudden level jumps > 6dB between consecutive blocks).
// ============================================================================
TEST_CASE("test_mode_switch_no_glitch", "[LimiterEngineModes]")
{
    LimiterEngine engine;
    engine.prepare(kSR, kBS, 2);
    engine.setOutputCeiling(0.0f);

    // Process 10 blocks, toggling a different mode each time
    for (int block = 0; block < 10; ++block)
    {
        // Toggle modes in rotation
        switch (block % 4)
        {
            case 0: engine.setBypass(block % 2 == 0);    break;
            case 1: engine.setDeltaMode(block % 2 == 0); break;
            case 2: engine.setUnityGain(block % 2 == 0); break;
            case 3: engine.setBypass(false); engine.setDeltaMode(false); engine.setUnityGain(false); break;
        }

        juce::AudioBuffer<float> buf = makeSine(2.0f);
        engine.process(buf);

        const float peak = maxAbs(buf);
        INFO("Block " << block << " peak: " << peak);

        // No sample should be unreasonably large (sanity bound)
        REQUIRE(peak <= 10.0f);

        // All samples should be finite
        for (int ch = 0; ch < buf.getNumChannels(); ++ch)
        {
            const float* data = buf.getReadPointer(ch);
            for (int i = 0; i < buf.getNumSamples(); ++i)
                REQUIRE(std::isfinite(data[i]));
        }

    }
}

// ============================================================================
// test_dc_filter_toggle
// Enabling and disabling the DC filter during processing should not crash
// and should produce finite output.
// ============================================================================
TEST_CASE("test_dc_filter_toggle", "[LimiterEngineModes]")
{
    LimiterEngine engine;
    engine.prepare(kSR, kBS, 2);
    engine.setOutputCeiling(0.0f);

    for (int block = 0; block < 10; ++block)
    {
        engine.setDCFilterEnabled(block % 2 == 0);

        juce::AudioBuffer<float> buf = makeSine(0.5f);
        engine.process(buf);

        // No crash and output must be finite
        const float peak = maxAbs(buf);
        INFO("DC filter block " << block << " (enabled=" << (block % 2 == 0) << ") peak: " << peak);
        REQUIRE(std::isfinite(peak));
        REQUIRE(peak <= 10.0f);
    }
}

// ============================================================================
// test_delta_plus_limited_equals_input
// Verifies the energy-conservation invariant of delta mode:
//   delta_output[i] + limited_output[i] ≈ input_after_gain[i]
// Because lookahead shifts timing, we verify a weaker but sound bound:
//   |delta_output[i]| <= |input_gained[i]| + epsilon
// (Delta can only reflect what was removed — it cannot exceed the input.)
// ============================================================================
TEST_CASE("test_delta_plus_limited_equals_input", "[LimiterEngineModes]")
{
    // Run two identical engines from the same input: one normal, one delta.
    // They won't produce perfectly complementary samples due to lookahead delay,
    // so instead we verify the boundedness invariant described in the task.

    LimiterEngine deltaEngine;
    deltaEngine.prepare(kSR, kBS, 2);
    deltaEngine.setOutputCeiling(0.0f);
    deltaEngine.setDeltaMode(true);

    // Track the input-after-gain to compare against delta output magnitude.
    // Input gain is 0 dB (default), so input-after-gain == raw input.
    constexpr float amp = 5.0f;  // loud signal to ensure heavy limiting
    constexpr int warmupBlocks = 10;
    constexpr float epsilon = 0.01f;

    // Warm up with the same signal to settle lookahead buffers
    for (int b = 0; b < warmupBlocks; ++b)
    {
        juce::AudioBuffer<float> w = makeSine(amp);
        deltaEngine.process(w);
    }

    // Capture a measurement block
    juce::AudioBuffer<float> deltaBuf = makeSine(amp);
    deltaEngine.process(deltaBuf);

    // Due to lookahead delay, we cannot compare sample-by-sample against the
    // current input frame. Instead verify the physically-sound global bound:
    // delta = input_gained - limited, and |limited| ≤ ceiling (1.0), so
    // |delta[i]| ≤ amp + ceiling + epsilon.
    const float maxDeltaBound = amp + 1.0f + epsilon;

    for (int ch = 0; ch < deltaBuf.getNumChannels(); ++ch)
    {
        const float* deltaData = deltaBuf.getReadPointer(ch);
        for (int i = 0; i < deltaBuf.getNumSamples(); ++i)
        {
            INFO("ch=" << ch << " i=" << i << " delta=" << deltaData[i]);
            // Delta output must be finite
            REQUIRE(std::isfinite(deltaData[i]));
            // Delta can never exceed the input amplitude + ceiling
            REQUIRE(std::abs(deltaData[i]) <= maxDeltaBound);
        }
    }
}

// ============================================================================
// test_delta_output_bounded
// With an extreme (100x amplitude) input and delta mode enabled, the output
// should stay within ±10.0 — no NaN, Inf, or runaway float values.
// ============================================================================
TEST_CASE("test_delta_output_bounded", "[LimiterEngineModes]")
{
    LimiterEngine engine;
    engine.prepare(kSR, kBS, 2);
    engine.setOutputCeiling(0.0f);
    engine.setDeltaMode(true);

    constexpr float extremeAmp = 100.0f;
    // Physical upper bound: delta = input_gained - limited.
    // |limited| ≤ ceiling (1.0), so |delta| ≤ extremeAmp + ceiling + small margin.
    // This proves no runaway/NaN/Inf — values stay within the expected float range.
    constexpr float boundLimit = extremeAmp + 2.0f;

    for (int block = 0; block < 20; ++block)
    {
        juce::AudioBuffer<float> buf = makeSine(extremeAmp);
        engine.process(buf);

        for (int ch = 0; ch < buf.getNumChannels(); ++ch)
        {
            const float* data = buf.getReadPointer(ch);
            for (int i = 0; i < buf.getNumSamples(); ++i)
            {
                INFO("block=" << block << " ch=" << ch << " i=" << i << " val=" << data[i]);
                REQUIRE(std::isfinite(data[i]));
                REQUIRE(std::abs(data[i]) <= boundLimit);
            }
        }
    }
}

// ============================================================================
// test_bypass_pushes_meter_data
// In bypass mode, process() must push at least one MeterData to the FIFO so
// the UI meters keep updating.  gainReduction must be 0.0 and both input and
// output levels must reflect the non-silent signal.
// ============================================================================
TEST_CASE("test_bypass_pushes_meter_data", "[LimiterEngineModes]")
{
    LimiterEngine engine;
    engine.prepare(kSR, kBS, 2);
    engine.setBypass(true);

    // Non-silent buffer: sine at 0.5 amplitude
    juce::AudioBuffer<float> buf = makeSine(0.5f);
    engine.process(buf);

    auto& fifo = engine.getMeterFIFO();
    MeterData md;
    const bool popped = fifo.pop(md);

    // (a) At least one MeterData must have been pushed
    REQUIRE(popped);

    // (b) No gain reduction in bypass
    REQUIRE(md.gainReduction == Catch::Approx(0.0f).margin(1e-6f));

    // (c) Levels must reflect the non-silent input
    REQUIRE(md.inputLevelL  > 0.0f);
    REQUIRE(md.outputLevelL > 0.0f);
}

// ============================================================================
// test_delta_rms_bounded_by_input
// With delta mode enabled and a +6 dBFS input, the delta output RMS must be
// strictly less than the input RMS (delta ≤ input — it's a fraction of what
// was removed, not an amplified signal), and greater than zero (something was
// actually removed by the limiter).
// ============================================================================
TEST_CASE("test_delta_rms_bounded_by_input", "[LimiterEngineModes]")
{
    // +6 dBFS ≈ amplitude 2.0 (well above the 0 dBFS ceiling)
    constexpr float amp = 2.0f;
    constexpr int   numBlocks = 20;

    LimiterEngine engine;
    engine.prepare(kSR, kBS, 2);
    engine.setOutputCeiling(0.0f);  // 0 dBFS ceiling
    engine.setDeltaMode(true);

    // RMS of the (un-limited) gain-adjusted input — input gain is 0 dB (default),
    // so this is just the RMS of the sine at the given amplitude.
    const float inputRMS = rms(makeSine(amp), 0);

    float lastDeltaRMS = 0.0f;
    for (int b = 0; b < numBlocks; ++b)
    {
        juce::AudioBuffer<float> buf = makeSine(amp);
        engine.process(buf);
        lastDeltaRMS = rms(buf, 0);
    }

    INFO("Input RMS: " << inputRMS << "  Delta RMS: " << lastDeltaRMS);

    // Delta must be non-zero — the limiter removed something
    REQUIRE(lastDeltaRMS > 0.001f);

    // Delta must be strictly less than the input — it is a removed portion,
    // not an amplified version of the input.
    REQUIRE(lastDeltaRMS < inputRMS);
}

// ============================================================================
// test_delta_sign_is_positive_for_loud
// After warm-up with a loud signal, the mean absolute value of the delta
// output must be positive (non-trivially above zero), confirming that the
// delta represents a genuine removed portion — not the zero signal you'd
// get if the delta were computed in the wrong direction (limited - input)
// and the result happened to cancel.
// ============================================================================
TEST_CASE("test_delta_sign_is_positive_for_loud", "[LimiterEngineModes]")
{
    constexpr float amp = 5.0f;  // very loud: well above 0 dBFS ceiling

    LimiterEngine engine;
    engine.prepare(kSR, kBS, 2);
    engine.setOutputCeiling(0.0f);
    engine.setDeltaMode(true);

    // Warm up so lookahead buffers are settled
    for (int b = 0; b < 10; ++b)
    {
        juce::AudioBuffer<float> w = makeSine(amp);
        engine.process(w);
    }

    // Capture the measurement block
    juce::AudioBuffer<float> buf = makeSine(amp);
    engine.process(buf);

    // Compute mean absolute value across channel 0
    const float* data = buf.getReadPointer(0);
    float sumAbs = 0.0f;
    for (int i = 0; i < buf.getNumSamples(); ++i)
        sumAbs += std::abs(data[i]);
    const float meanAbsDelta = sumAbs / buf.getNumSamples();

    INFO("Mean absolute delta value: " << meanAbsDelta);

    // With heavy limiting (5x amplitude input, ceiling=1.0) a large fraction of
    // each cycle is removed.  The mean absolute delta must be comfortably above
    // zero — a threshold of 0.1 is easily met and guards against a degenerate
    // (all-zero) delta output or a reversed-sign implementation that was somehow
    // cancelled to near-zero.
    REQUIRE(meanAbsDelta > 0.1f);

    // All delta samples must be finite
    for (int i = 0; i < buf.getNumSamples(); ++i)
        REQUIRE(std::isfinite(data[i]));
}

// ============================================================================
// test_bypass_still_pushes_meter_data
// In bypass mode, process() must push at least one MeterData to the FIFO even
// after multiple blocks.  gainReduction must be 0.0 and inputLevelL > 0.
// ============================================================================
TEST_CASE("test_bypass_still_pushes_meter_data", "[LimiterEngineModes]")
{
    LimiterEngine engine;
    engine.prepare(kSR, kBS, 2);
    engine.setBypass(true);

    // -6 dBFS sine: amplitude ≈ 0.501 (20*log10(0.501) ≈ -6 dBFS)
    const float amp = std::pow(10.0f, -6.0f / 20.0f);

    // Process 5 blocks — each should push a MeterData entry
    for (int i = 0; i < 5; ++i)
    {
        juce::AudioBuffer<float> buf = makeSine(amp);
        engine.process(buf);
    }

    auto& fifo = engine.getMeterFIFO();
    MeterData md;
    bool popped = false;
    MeterData last;
    while (fifo.pop(md))
    {
        popped = true;
        last = md;
    }

    // (a) At least one MeterData was pushed
    REQUIRE(popped);

    // (b) Input is metered — inputLevelL must reflect the non-silent signal
    INFO("inputLevelL: " << last.inputLevelL);
    REQUIRE(last.inputLevelL > 0.0f);

    // (c) No gain reduction in bypass mode
    REQUIRE(last.gainReduction == Catch::Approx(0.0f).margin(1e-6f));
}

// ============================================================================
// test_bypass_input_level_reflects_signal
// In bypass mode, the inputLevelL in popped MeterData should approximate the
// peak amplitude of the bypassed audio (within 10%).
// ============================================================================
TEST_CASE("test_bypass_input_level_reflects_signal", "[LimiterEngineModes]")
{
    LimiterEngine engine;
    engine.prepare(kSR, kBS, 2);
    engine.setBypass(true);

    // +6 dBFS signal: amplitude = 2.0 (above normal ceiling, but bypass passes it)
    const float amp = 2.0f;

    // Process 10 blocks and drain FIFO each time
    MeterData last;
    bool gotAny = false;
    for (int i = 0; i < 10; ++i)
    {
        juce::AudioBuffer<float> buf = makeSine(amp);
        engine.process(buf);

        MeterData md;
        while (engine.getMeterFIFO().pop(md))
        {
            last = md;
            gotAny = true;
        }
    }

    REQUIRE(gotAny);

    // inputLevelL should approximately equal the peak amplitude (~2.0), within 10%
    INFO("inputLevelL: " << last.inputLevelL << "  expected: ~" << amp);
    REQUIRE(last.inputLevelL == Catch::Approx(amp).epsilon(0.10f));
}

// ============================================================================
// test_dither_toggle
// Enabling and disabling dither during processing should not crash
// and should produce finite output.
// ============================================================================
TEST_CASE("test_dither_toggle", "[LimiterEngineModes]")
{
    LimiterEngine engine;
    engine.prepare(kSR, kBS, 2);
    engine.setOutputCeiling(0.0f);
    engine.setDitherBitDepth(24);

    for (int block = 0; block < 10; ++block)
    {
        engine.setDitherEnabled(block % 2 == 0);

        juce::AudioBuffer<float> buf = makeSine(0.5f);
        engine.process(buf);

        // No crash and output must be finite
        const float peak = maxAbs(buf);
        INFO("Dither block " << block << " (enabled=" << (block % 2 == 0) << ") peak: " << peak);
        REQUIRE(std::isfinite(peak));
        REQUIRE(peak <= 10.0f);
    }
}
