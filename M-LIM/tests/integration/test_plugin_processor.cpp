/**
 * test_plugin_processor.cpp — PluginProcessor integration tests (Task 017)
 *
 * Tests that MLIMAudioProcessor wires up correctly: processes audio, limits
 * to ceiling, saves/restores state, and reports non-zero latency.
 */
#include "catch2/catch_amalgamated.hpp"

#include "PluginProcessor.h"
#include "PluginEditor.h"
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
// test_oversampling_change_no_audio_glitch
// Changing the oversamplingFactor parameter while processing must not crash
// or trigger assertions. The deferred AsyncUpdater mechanism handles the
// actual reallocation off the audio thread.
// ============================================================================
TEST_CASE("test_oversampling_change_no_audio_glitch", "[PluginProcessor]")
{
    MLIMAudioProcessor proc;
    proc.prepareToPlay (kSampleRate, kBlockSize);

    juce::AudioBuffer<float> buffer (kNumChannels, kBlockSize);
    juce::MidiBuffer midiBuffer;

    // Process some blocks at the default oversampling factor (0 = off)
    for (int i = 0; i < 10; ++i)
    {
        fillSine (buffer, 0.5f);
        proc.processBlock (buffer, midiBuffer);
    }

    // Change oversampling factor to 2x (factor index 1) via the parameter
    if (auto* param = proc.apvts.getParameter (ParamID::oversamplingFactor))
        param->setValueNotifyingHost (param->convertTo0to1 (1.0f));

    // Process more blocks — should not crash or assert (rebuild is deferred)
    for (int i = 0; i < 10; ++i)
    {
        fillSine (buffer, 0.5f);
        proc.processBlock (buffer, midiBuffer);
    }

    // Simulate host calling prepareToPlay after the factor change
    // (this is how many hosts apply deferred changes)
    proc.prepareToPlay (kSampleRate, kBlockSize);

    // Process more blocks with new oversampling active
    for (int i = 0; i < 20; ++i)
    {
        fillSine (buffer, 0.5f);
        proc.processBlock (buffer, midiBuffer);

        // Output must remain finite (no NaN/inf from the oversampler)
        bool allFinite = true;
        for (int ch = 0; ch < buffer.getNumChannels() && allFinite; ++ch)
        {
            const float* data = buffer.getReadPointer (ch);
            for (int s = 0; s < buffer.getNumSamples(); ++s)
                if (!std::isfinite (data[s])) { allFinite = false; break; }
        }
        REQUIRE (allFinite);
    }
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

// ============================================================================
// test_tail_length_reflects_lookahead
// getTailLengthSeconds() must return a value >= the current lookahead time so
// the host sends extra silent blocks to drain the lookahead buffer.
// ============================================================================
TEST_CASE("test_tail_length_reflects_lookahead", "[PluginProcessor]")
{
    MLIMAudioProcessor proc;
    proc.prepareToPlay (kSampleRate, kBlockSize);

    // Set lookahead to 2 ms
    if (auto* param = proc.apvts.getParameter (ParamID::lookahead))
        param->setValueNotifyingHost (param->convertTo0to1 (2.0f));

    REQUIRE (proc.getTailLengthSeconds() >= 0.002);
}

// ============================================================================
// test_oversampling_change_loudness_meter_reprepared
// After an oversampling factor change via handleAsyncUpdate, the loudness meter
// must still function correctly (no crash, finite LUFS values).
// ============================================================================
TEST_CASE("test_oversampling_change_loudness_meter_reprepared", "[PluginProcessor]")
{
    MLIMAudioProcessor proc;
    proc.prepareToPlay (kSampleRate, kBlockSize);

    juce::AudioBuffer<float> buffer (kNumChannels, kBlockSize);
    juce::MidiBuffer midiBuffer;

    // Process some audio at the default oversampling setting
    for (int i = 0; i < 20; ++i)
    {
        fillSine (buffer, 0.5f);
        proc.processBlock (buffer, midiBuffer);
    }

    // Change oversampling factor to 2x (factor index 1) — triggers parameterChanged
    // which schedules handleAsyncUpdate via triggerAsyncUpdate()
    if (auto* param = proc.apvts.getParameter (ParamID::oversamplingFactor))
        param->setValueNotifyingHost (param->convertTo0to1 (1.0f));

    // Simulate the host calling prepareToPlay after the deferred change
    // (many hosts call prepareToPlay when graph topology changes)
    proc.prepareToPlay (kSampleRate, kBlockSize);

    // Process more blocks — loudness meter must not crash or produce NaN
    for (int i = 0; i < 50; ++i)
    {
        fillSine (buffer, 0.5f);
        proc.processBlock (buffer, midiBuffer);
    }

    // getLoudnessMeter() must return a finite (non-NaN, non-inf) value
    const float momentary = proc.getLoudnessMeter().getMomentaryLUFS();
    REQUIRE (std::isfinite (momentary));
}

// ============================================================================
// test_latency_updates_with_lookahead
// Changing the lookahead parameter must update getLatencySamples() to reflect
// the new lookahead time. The host needs accurate latency for delay compensation.
// ============================================================================
TEST_CASE("test_latency_updates_with_lookahead", "[PluginProcessor]")
{
    MLIMAudioProcessor proc;
    proc.prepareToPlay (kSampleRate, kBlockSize);

    // Set lookahead to 5 ms — parameterChanged fires synchronously and
    // calls updateLatency(), so getLatencySamples() should reflect this.
    if (auto* param = proc.apvts.getParameter (ParamID::lookahead))
        param->setValueNotifyingHost (param->convertTo0to1 (5.0f));

    const int expected5ms = static_cast<int> (5.0 * kSampleRate / 1000.0); // 220 @ 44100
    const int latency5ms  = proc.getLatencySamples();

    // Allow ±1 sample rounding in the integer conversion
    REQUIRE (latency5ms >= expected5ms - 1);

    // Change to 0 ms lookahead
    if (auto* param = proc.apvts.getParameter (ParamID::lookahead))
        param->setValueNotifyingHost (param->convertTo0to1 (0.0f));

    const int latency0ms = proc.getLatencySamples();

    // Latency at 0 ms must be less than at 5 ms
    REQUIRE (latency0ms < latency5ms);
}

// ============================================================================
// test_state_round_trip_all_params
// Set all 21 parameters to non-default values, save state, create a new
// processor, load state, and verify every parameter is restored correctly.
// ============================================================================
TEST_CASE("test_state_round_trip_all_params", "[PluginProcessor]")
{
    struct ParamTestValue { const char* id; float value; };

    // One non-default value per parameter (real/denormalized units or index)
    const std::vector<ParamTestValue> testValues = {
        { "inputGain",              6.0f   },
        { "outputCeiling",         -1.0f   },
        { "algorithm",              3.0f   },   // index 3 = Aggressive
        { "lookahead",              3.0f   },
        { "attack",                10.0f   },
        { "release",              200.0f   },
        { "channelLinkTransients", 50.0f   },
        { "channelLinkRelease",    50.0f   },
        { "truePeakEnabled",        0.0f   },   // default true → false
        { "oversamplingFactor",     1.0f   },   // index 1 = 2x
        { "dcFilterEnabled",        1.0f   },
        { "ditherEnabled",          1.0f   },
        { "ditherBitDepth",         2.0f   },   // index 2 = 20-bit
        { "ditherNoiseShaping",     1.0f   },   // index 1 = Optimized
        { "bypass",                 1.0f   },
        { "unityGainMode",          1.0f   },
        { "sidechainHPFreq",      200.0f   },
        { "sidechainLPFreq",    10000.0f   },
        { "sidechainTilt",          3.0f   },
        { "delta",                  1.0f   },
        { "displayMode",            2.0f   },   // index 2 = SlowDown
    };

    MLIMAudioProcessor procA;
    procA.prepareToPlay (kSampleRate, kBlockSize);

    for (const auto& tv : testValues)
    {
        auto* param = procA.apvts.getParameter (tv.id);
        REQUIRE (param != nullptr);
        param->setValueNotifyingHost (param->convertTo0to1 (tv.value));
    }

    juce::MemoryBlock stateData;
    procA.getStateInformation (stateData);
    REQUIRE (stateData.getSize() > 0);

    MLIMAudioProcessor procB;
    procB.setStateInformation (stateData.getData(),
                               static_cast<int> (stateData.getSize()));

    for (const auto& tv : testValues)
    {
        auto* raw = procB.apvts.getRawParameterValue (tv.id);
        REQUIRE (raw != nullptr);
        REQUIRE (raw->load() == Catch::Approx (tv.value).margin (0.01f));
    }
}

// ============================================================================
// test_bus_layout_stereo_supported
// isBusesLayoutSupported() must return true for matching stereo in/out and
// false for mismatched channel sets.
// ============================================================================
TEST_CASE("test_bus_layout_stereo_supported", "[PluginProcessor]")
{
    MLIMAudioProcessor proc;

    juce::AudioProcessor::BusesLayout stereoLayout;
    stereoLayout.inputBuses.add  (juce::AudioChannelSet::stereo());
    stereoLayout.outputBuses.add (juce::AudioChannelSet::stereo());
    REQUIRE (proc.isBusesLayoutSupported (stereoLayout) == true);

    // Mismatched: stereo in, mono out — must be rejected
    juce::AudioProcessor::BusesLayout mismatchedLayout;
    mismatchedLayout.inputBuses.add  (juce::AudioChannelSet::stereo());
    mismatchedLayout.outputBuses.add (juce::AudioChannelSet::mono());
    REQUIRE (proc.isBusesLayoutSupported (mismatchedLayout) == false);
}

// ============================================================================
// test_tail_length_positive
// After prepareToPlay(), getTailLengthSeconds() must be > 0 because the
// default lookahead is 1 ms.
// ============================================================================
TEST_CASE("test_tail_length_positive", "[PluginProcessor]")
{
    MLIMAudioProcessor proc;
    proc.prepareToPlay (kSampleRate, kBlockSize);

    // Default lookahead is 1 ms → tail = 0.001 s > 0
    REQUIRE (proc.getTailLengthSeconds() > 0.0);
}

// ============================================================================
// test_set_state_empty_no_crash
// Passing nullptr / zero size to setStateInformation() must not crash.
// ============================================================================
TEST_CASE("test_set_state_empty_no_crash", "[PluginProcessor]")
{
    MLIMAudioProcessor proc;
    REQUIRE_NOTHROW (proc.setStateInformation (nullptr, 0));
}

// ============================================================================
// test_set_state_garbage_no_crash
// Passing 64 bytes of arbitrary data to setStateInformation() must not crash.
// ============================================================================
TEST_CASE("test_set_state_garbage_no_crash", "[PluginProcessor]")
{
    MLIMAudioProcessor proc;

    std::vector<char> garbage (64);
    for (int i = 0; i < 64; ++i)
        garbage[i] = static_cast<char> (i * 7 + 3);

    REQUIRE_NOTHROW (proc.setStateInformation (garbage.data(),
                                               static_cast<int> (garbage.size())));
}

// ============================================================================
// test_smaller_block_sizes_no_crash
// After prepareToPlay(44100, 512), call processBlock() with blocks of 1, 16,
// 64, 128, and 256 samples. Output must be finite and not crash.
// ============================================================================
TEST_CASE("test_smaller_block_sizes_no_crash", "[PluginProcessor]")
{
    MLIMAudioProcessor proc;
    proc.prepareToPlay (44100.0, 512);

    juce::MidiBuffer midi;
    for (int blockSize : {1, 16, 64, 128, 256})
    {
        juce::AudioBuffer<float> buf (2, blockSize);
        for (int ch = 0; ch < 2; ++ch)
            for (int i = 0; i < blockSize; ++i)
                buf.setSample (ch, i, 2.0f * std::sin (6.28f * 440.0f * i / 44100.0f));

        REQUIRE_NOTHROW (proc.processBlock (buf, midi));

        for (int ch = 0; ch < 2; ++ch)
            for (int i = 0; i < blockSize; ++i)
                REQUIRE (std::isfinite (buf.getSample (ch, i)));
    }
}

// ============================================================================
// test_mixed_block_sizes_bounded
// Alternate between 512 and 64-sample blocks across 50 total process calls.
// Output must remain finite and never exceed the output ceiling.
// ============================================================================
TEST_CASE("test_mixed_block_sizes_bounded", "[PluginProcessor]")
{
    MLIMAudioProcessor proc;
    proc.prepareToPlay (44100.0, 512);

    // Default ceiling is -0.1 dBFS ≈ 0.9886 linear; use 1% tolerance for ringing
    const float ceilingDb     = -0.1f;
    const float ceilingLinear = std::pow (10.0f, ceilingDb / 20.0f);
    const float tolerance     = 0.01f;

    juce::MidiBuffer midi;

    for (int call = 0; call < 50; ++call)
    {
        const int blockSize = (call % 2 == 0) ? 512 : 64;
        juce::AudioBuffer<float> buf (2, blockSize);
        for (int ch = 0; ch < 2; ++ch)
            for (int i = 0; i < blockSize; ++i)
                buf.setSample (ch, i, 2.0f * std::sin (6.28f * 440.0f * i / 44100.0f));

        REQUIRE_NOTHROW (proc.processBlock (buf, midi));

        // Skip the first few calls to allow the limiter to engage
        if (call > 10)
        {
            for (int ch = 0; ch < 2; ++ch)
                for (int i = 0; i < blockSize; ++i)
                {
                    REQUIRE (std::isfinite (buf.getSample (ch, i)));
                    REQUIRE (std::abs (buf.getSample (ch, i)) <= ceilingLinear + tolerance);
                }
        }
    }
}

// ============================================================================
// test_single_sample_block
// processBlock() with a 1-sample buffer must not crash and output must be
// finite and bounded by the ceiling.
// ============================================================================
TEST_CASE("test_single_sample_block", "[PluginProcessor]")
{
    MLIMAudioProcessor proc;
    proc.prepareToPlay (44100.0, 512);

    const float ceilingDb     = -0.1f;
    const float ceilingLinear = std::pow (10.0f, ceilingDb / 20.0f);
    const float tolerance     = 0.01f;

    juce::MidiBuffer midi;

    // Process 100 single-sample blocks to allow limiter to engage
    for (int call = 0; call < 100; ++call)
    {
        juce::AudioBuffer<float> buf (2, 1);
        buf.setSample (0, 0, 2.0f);
        buf.setSample (1, 0, 2.0f);

        REQUIRE_NOTHROW (proc.processBlock (buf, midi));

        REQUIRE (std::isfinite (buf.getSample (0, 0)));
        REQUIRE (std::isfinite (buf.getSample (1, 0)));

        if (call > 10)
        {
            REQUIRE (std::abs (buf.getSample (0, 0)) <= ceilingLinear + tolerance);
            REQUIRE (std::abs (buf.getSample (1, 0)) <= ceilingLinear + tolerance);
        }
    }
}

// ============================================================================
// test_accepts_midi_false
// A limiter plugin must not accept MIDI input.
// ============================================================================
TEST_CASE("test_accepts_midi_false", "[PluginProcessor]")
{
    MLIMAudioProcessor proc;
    REQUIRE (proc.acceptsMidi() == false);
}

// ============================================================================
// test_editor_create_destroy_no_crash
// Create and destroy MLIMAudioProcessorEditor multiple times.  Verifies that
// the LookAndFeel lifecycle is correct: MLIMLookAndFeel is declared before
// child components (so it outlives them), and the destructor calls
// setLookAndFeel(nullptr) before children are destroyed — preventing UB from
// accessing a deleted LookAndFeel object during component teardown.
// ============================================================================
TEST_CASE("test_editor_create_destroy_no_crash", "[PluginProcessor][Editor]")
{
    // MessageManager is required for Component construction and Timer operations
    juce::MessageManager::getInstance();

    for (int i = 0; i < 5; ++i)
    {
        MLIMAudioProcessor proc;
        proc.prepareToPlay (kSampleRate, kBlockSize);

        // Construct and immediately destroy the editor.  If the LookAndFeel is
        // destroyed before child components the JUCE LookAndFeel system asserts
        // or dereferences a dangling pointer — this loop would crash or fire an
        // assertion failure in that case.
        REQUIRE_NOTHROW (
            [&]()
            {
                MLIMAudioProcessorEditor editor (proc);
                // editor goes out of scope here — destructor calls
                // setLookAndFeel(nullptr), then child components are destroyed,
                // then lookAndFeel_ member is destroyed last (declared first).
            }()
        );
    }
}

// ============================================================================
// test_loudness_target_syncs_to_panel_on_state_restore
// Create a processor+editor, then set loudnessTarget via the APVTS parameter
// and verify that LoudnessPanel reflects the new choice.
// ============================================================================
TEST_CASE("test_loudness_target_syncs_to_panel_on_state_restore", "[PluginProcessor][Editor]")
{
    juce::MessageManager::getInstance();

    MLIMAudioProcessor proc;
    proc.prepareToPlay (kSampleRate, kBlockSize);

    MLIMAudioProcessorEditor editor (proc);

    // Set loudnessTarget to index 3 — this fires the APVTS listener which calls
    // loudnessPanel_.setTargetChoice(3)
    auto* param = proc.apvts.getParameter (ParamID::loudnessTarget);
    REQUIRE (param != nullptr);
    param->setValueNotifyingHost (param->convertTo0to1 (3.0f));

    // The APVTS listener is called synchronously on the message thread, so the
    // panel should be updated immediately.
    REQUIRE (editor.getLoudnessPanel().getTargetChoice() == 3);
}

// ============================================================================
// test_tail_length_includes_oversampler
// After prepareToPlay with 2x oversampling, getTailLengthSeconds() must be
// >= getLatencySamples() / sampleRate (i.e. includes oversampler latency).
// ============================================================================
TEST_CASE("test_tail_length_includes_oversampler", "[PluginProcessor]")
{
    MLIMAudioProcessor proc;

    // Enable 2x oversampling via the parameter before prepare
    if (auto* p = proc.apvts.getParameter (ParamID::oversamplingFactor))
        p->setValueNotifyingHost (p->convertTo0to1 (1.0f)); // factor = 1 → 2x

    proc.prepareToPlay (kSampleRate, kBlockSize);

    const int    latencySamples  = proc.getLatencySamples();
    const double tailSec         = proc.getTailLengthSeconds();
    const double expectedMinSec  = static_cast<double>(latencySamples) / kSampleRate - 0.001;

    INFO("latencySamples=" << latencySamples << " tailSec=" << tailSec
         << " expectedMin=" << expectedMinSec);

    REQUIRE(latencySamples >= 0);
    REQUIRE(tailSec >= expectedMinSec);
}

// ============================================================================
// test_display_mode_syncs_to_waveform_on_state_restore
// Save state with displayMode = 3 (Infinite), create a new processor+editor,
// load that state, and verify WaveformDisplay reflects DisplayMode::Infinite.
// ============================================================================
TEST_CASE("test_display_mode_syncs_to_waveform_on_state_restore", "[PluginProcessor][Editor]")
{
    juce::MessageManager::getInstance();

    // Processor A: set displayMode to 3 (Infinite) and save state
    MLIMAudioProcessor procA;
    procA.prepareToPlay (kSampleRate, kBlockSize);

    auto* paramA = procA.apvts.getParameter (ParamID::displayMode);
    REQUIRE (paramA != nullptr);
    paramA->setValueNotifyingHost (paramA->convertTo0to1 (3.0f));

    juce::MemoryBlock stateData;
    procA.getStateInformation (stateData);
    REQUIRE (stateData.getSize() > 0);

    // Processor B: load saved state, then create editor
    MLIMAudioProcessor procB;
    procB.setStateInformation (stateData.getData(),
                               static_cast<int> (stateData.getSize()));

    // APVTS parameter must be restored to 3 (Infinite)
    auto* rawB = procB.apvts.getRawParameterValue (ParamID::displayMode);
    REQUIRE (rawB != nullptr);
    REQUIRE (rawB->load() == Catch::Approx (3.0f).margin (0.01f));

    // Editor constructor syncs WaveformDisplay from APVTS initial value
    MLIMAudioProcessorEditor editor (procB);
    REQUIRE (editor.getWaveformDisplay().getDisplayMode()
             == WaveformDisplay::DisplayMode::Infinite);
}

// ============================================================================
// test_set_state_wrong_tag_leaves_state_unchanged
// setStateInformation() with valid XML but a mismatched root tag must NOT
// alter the current parameter state.
// ============================================================================
TEST_CASE("test_set_state_wrong_tag_leaves_state_unchanged", "[PluginProcessor]")
{
    MLIMAudioProcessor proc;

    // Set inputGain to a known non-default value (6 dB)
    if (auto* param = proc.apvts.getParameter (ParamID::inputGain))
        param->setValueNotifyingHost (param->convertTo0to1 (6.0f));

    auto* rawGain = proc.apvts.getRawParameterValue (ParamID::inputGain);
    REQUIRE (rawGain != nullptr);
    REQUIRE (rawGain->load() == Catch::Approx (6.0f).margin (0.01f));

    // Build a valid XML blob whose root tag deliberately does NOT match the
    // APVTS state type (which is "MLIMParameters").
    auto wrongXml = std::make_unique<juce::XmlElement> ("WrongTag");
    wrongXml->setAttribute ("inputGain", "0.0");  // different value in the XML

    juce::MemoryBlock wrongBlock;
    juce::AudioProcessor::copyXmlToBinary (*wrongXml, wrongBlock);

    // Loading wrong-tag state must be silently ignored
    REQUIRE_NOTHROW (proc.setStateInformation (wrongBlock.getData(),
                                               static_cast<int> (wrongBlock.getSize())));

    // inputGain must still be 6.0 — not replaced by the XML's 0.0
    REQUIRE (rawGain->load() == Catch::Approx (6.0f).margin (0.01f));
}

// ============================================================================
// test_set_state_correct_tag_replaces_state
// setStateInformation() with valid XML and the matching root tag MUST replace
// the current parameter state.
// ============================================================================
TEST_CASE("test_set_state_correct_tag_replaces_state", "[PluginProcessor]")
{
    // Processor A: set inputGain = 12 dB and export state
    MLIMAudioProcessor procA;
    if (auto* param = procA.apvts.getParameter (ParamID::inputGain))
        param->setValueNotifyingHost (param->convertTo0to1 (12.0f));

    juce::MemoryBlock stateData;
    procA.getStateInformation (stateData);
    REQUIRE (stateData.getSize() > 0);

    // Processor B: start with default inputGain (0 dB), then load procA's state
    MLIMAudioProcessor procB;
    auto* rawGainB = procB.apvts.getRawParameterValue (ParamID::inputGain);
    REQUIRE (rawGainB != nullptr);
    REQUIRE (rawGainB->load() == Catch::Approx (0.0f).margin (0.01f));

    procB.setStateInformation (stateData.getData(),
                               static_cast<int> (stateData.getSize()));

    // State must now reflect procA's value
    REQUIRE (rawGainB->load() == Catch::Approx (12.0f).margin (0.01f));
}
