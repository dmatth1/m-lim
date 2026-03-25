/**
 * test_plugin_processor.cpp — PluginProcessor integration tests (Task 017)
 *
 * Tests that MLIMAudioProcessor wires up correctly: processes audio, limits
 * to ceiling, saves/restores state, and reports non-zero latency.
 */
#include "catch2/catch_amalgamated.hpp"

#include "PluginProcessor.h"
#include "Parameters.h"

#include <juce_audio_processors/juce_audio_processors.h>
#include <cmath>
#include <limits>
#include <vector>

// ---------------------------------------------------------------------------
// Test helpers
// ---------------------------------------------------------------------------

static constexpr double kSampleRate  = 44100.0;
static constexpr int    kBlockSize   = 512;
static constexpr int    kNumChannels = 2;

/** Fill a stereo buffer with a sine wave at the given amplitude (linear). */
static void fillSine (juce::AudioBuffer<float>& buf, float amplitude, double freq = 1000.0)
{
    const double phaseStep = 2.0 * juce::MathConstants<double>::pi
                             * freq / kSampleRate;
    for (int ch = 0; ch < buf.getNumChannels(); ++ch)
    {
        float* data = buf.getWritePointer (ch);
        for (int i = 0; i < buf.getNumSamples(); ++i)
            data[i] = amplitude * static_cast<float> (std::sin (phaseStep * i));
    }
}

/** Returns the peak absolute sample value across all channels. */
static float peakLevel (const juce::AudioBuffer<float>& buf)
{
    float peak = 0.0f;
    for (int ch = 0; ch < buf.getNumChannels(); ++ch)
    {
        const float* data = buf.getReadPointer (ch);
        for (int i = 0; i < buf.getNumSamples(); ++i)
            peak = std::max (peak, std::abs (data[i]));
    }
    return peak;
}

// ============================================================================
// test_process_block_no_crash
// Process 1000 blocks through the plugin processor without crashing.
// ============================================================================
TEST_CASE("test_process_block_no_crash", "[PluginProcessor]")
{
    MLIMAudioProcessor proc;
    proc.prepareToPlay (kSampleRate, kBlockSize);

    juce::AudioBuffer<float> buffer (kNumChannels, kBlockSize);
    juce::MidiBuffer midiBuffer;

    for (int block = 0; block < 1000; ++block)
    {
        fillSine (buffer, 0.5f);
        proc.processBlock (buffer, midiBuffer);
    }

    // If we reach here without crashing, the test passes.
    REQUIRE (true);
}

// ============================================================================
// test_output_within_ceiling
// Feed a loud sine wave; output must not exceed the ceiling parameter.
// ============================================================================
TEST_CASE("test_output_within_ceiling", "[PluginProcessor]")
{
    MLIMAudioProcessor proc;
    proc.prepareToPlay (kSampleRate, kBlockSize);

    // Default ceiling is -0.1 dBFS ≈ 0.9886 linear
    const float ceilingDb     = -0.1f;
    const float ceilingLinear = std::pow (10.0f, ceilingDb / 20.0f);
    const float tolerance     = 0.01f; // 1% headroom for filter ringing

    juce::AudioBuffer<float> buffer (kNumChannels, kBlockSize);
    juce::MidiBuffer midiBuffer;

    // Send a loud signal (6 dBFS over-threshold) through the limiter
    for (int block = 0; block < 200; ++block)
    {
        fillSine (buffer, 2.0f); // 6 dB over 0 dBFS
        proc.processBlock (buffer, midiBuffer);

        // Skip the first few blocks to allow the limiter to engage
        if (block > 10)
        {
            const float peak = peakLevel (buffer);
            REQUIRE (peak <= ceilingLinear + tolerance);
        }
    }
}

// ============================================================================
// test_state_save_load
// Save state, create a second processor, load state, verify parameters match.
// ============================================================================
TEST_CASE("test_state_save_load", "[PluginProcessor]")
{
    // Set non-default parameter values in processor A
    MLIMAudioProcessor procA;
    procA.prepareToPlay (kSampleRate, kBlockSize);

    // Modify some parameters
    if (auto* param = procA.apvts.getParameter (ParamID::inputGain))
        param->setValueNotifyingHost (param->convertTo0to1 (6.0f));

    if (auto* param = procA.apvts.getParameter (ParamID::outputCeiling))
        param->setValueNotifyingHost (param->convertTo0to1 (-1.0f));

    if (auto* param = procA.apvts.getParameter (ParamID::lookahead))
        param->setValueNotifyingHost (param->convertTo0to1 (2.5f));

    // Serialize state
    juce::MemoryBlock stateData;
    procA.getStateInformation (stateData);

    REQUIRE (stateData.getSize() > 0);

    // Load state into processor B and verify parameters match
    MLIMAudioProcessor procB;
    procB.setStateInformation (stateData.getData(),
                               static_cast<int> (stateData.getSize()));

    // Check that key parameters were restored
    auto* inputGainB = procB.apvts.getRawParameterValue (ParamID::inputGain);
    auto* ceilingB   = procB.apvts.getRawParameterValue (ParamID::outputCeiling);
    auto* lookaheadB = procB.apvts.getRawParameterValue (ParamID::lookahead);

    REQUIRE (inputGainB  != nullptr);
    REQUIRE (ceilingB    != nullptr);
    REQUIRE (lookaheadB  != nullptr);

    REQUIRE (inputGainB->load()  == Catch::Approx (6.0f).margin (0.01f));
    REQUIRE (ceilingB->load()    == Catch::Approx (-1.0f).margin (0.01f));
    REQUIRE (lookaheadB->load()  == Catch::Approx (2.5f).margin (0.01f));
}

// ============================================================================
// test_loudness_metering_active
// Process enough sine wave blocks to accumulate LUFS data and verify the
// loudness meter reports a finite, non-silence value.
// ============================================================================
TEST_CASE("test_loudness_metering_active", "[PluginProcessor]")
{
    MLIMAudioProcessor proc;
    proc.prepareToPlay (kSampleRate, kBlockSize);

    juce::AudioBuffer<float> buffer (kNumChannels, kBlockSize);
    juce::MidiBuffer midiBuffer;

    // Need > 400 ms for momentary window to fill: 400 ms @ 44100 / 512 = ~35 blocks.
    // Process 100 to be safe.
    for (int block = 0; block < 100; ++block)
    {
        fillSine (buffer, 0.5f); // -6 dBFS sine
        proc.processBlock (buffer, midiBuffer);
    }

    const float momentary = proc.getLoudnessMeter().getMomentaryLUFS();

    // Must be a valid finite LUFS value (not -infinity or NaN)
    REQUIRE (std::isfinite (momentary));
    // A -6 dBFS sine should produce a loudness well above silence
    REQUIRE (momentary < 0.0f);
    REQUIRE (momentary > -100.0f);
}

// ============================================================================
// test_latency_reported
// When lookahead > 0, getLatencyInSamples() must be > 0.
// ============================================================================
TEST_CASE("test_latency_reported", "[PluginProcessor]")
{
    MLIMAudioProcessor proc;

    // Set lookahead to 1 ms
    if (auto* param = proc.apvts.getParameter (ParamID::lookahead))
        param->setValueNotifyingHost (param->convertTo0to1 (1.0f));

    proc.prepareToPlay (kSampleRate, kBlockSize);

    REQUIRE (proc.getLatencySamples() > 0);
}
