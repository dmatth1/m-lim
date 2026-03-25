/**
 * test_processor_stress.cpp — Plugin Processor Stress and Robustness Tests (Task 055)
 *
 * Tests rapid parameter automation, buffer size changes, prepare/release cycles,
 * all-features-enabled processing, all algorithms, and sample rate switching.
 */
#include "catch2/catch_amalgamated.hpp"

#include "PluginProcessor.h"
#include "Parameters.h"

#include <juce_audio_processors/juce_audio_processors.h>
#include <cmath>
#include <vector>

// ---------------------------------------------------------------------------
// Helpers
// ---------------------------------------------------------------------------

static constexpr double kSampleRate  = 44100.0;
static constexpr int    kBlockSize   = 512;
static constexpr int    kNumChannels = 2;

/** Fill buffer with pink-noise-like signal at given amplitude. */
static void fillPinkNoise (juce::AudioBuffer<float>& buf, float amplitude, juce::Random& rng)
{
    // Simple white noise approximation (adequate for stress tests)
    for (int ch = 0; ch < buf.getNumChannels(); ++ch)
    {
        float* data = buf.getWritePointer (ch);
        for (int i = 0; i < buf.getNumSamples(); ++i)
            data[i] = amplitude * (rng.nextFloat() * 2.0f - 1.0f);
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

/** Returns true if every sample in the buffer is finite. */
static bool allFinite (const juce::AudioBuffer<float>& buf)
{
    for (int ch = 0; ch < buf.getNumChannels(); ++ch)
    {
        const float* data = buf.getReadPointer (ch);
        for (int i = 0; i < buf.getNumSamples(); ++i)
            if (!std::isfinite (data[i]))
                return false;
    }
    return true;
}

// ============================================================================
// test_rapid_parameter_changes
// Change every parameter to random valid normalised values between each
// processBlock call for 500 blocks, verify no crash and output is finite.
// ============================================================================
TEST_CASE("test_rapid_parameter_changes", "[ProcessorStress]")
{
    MLIMAudioProcessor proc;
    proc.prepareToPlay (kSampleRate, kBlockSize);

    juce::Random rng (42); // fixed seed for reproducibility

    juce::AudioBuffer<float> buffer (kNumChannels, kBlockSize);
    juce::MidiBuffer midiBuffer;

    // Collect all parameter IDs to automate
    const std::vector<juce::String> paramIDs {
        ParamID::inputGain,
        ParamID::outputCeiling,
        ParamID::algorithm,
        ParamID::lookahead,
        ParamID::attack,
        ParamID::release,
        ParamID::channelLinkTransients,
        ParamID::channelLinkRelease,
        ParamID::truePeakEnabled,
        ParamID::dcFilterEnabled,
        ParamID::ditherEnabled,
        ParamID::ditherBitDepth,
        ParamID::ditherNoiseShaping,
        ParamID::bypass,
        ParamID::unityGainMode,
        ParamID::sidechainHPFreq,
        ParamID::sidechainLPFreq,
        ParamID::sidechainTilt,
        ParamID::delta,
        ParamID::displayMode
        // Note: oversamplingFactor is intentionally excluded; its change is
        // deferred to the next prepareToPlay call via AsyncUpdater and is
        // covered by test_all_features_enabled.
    };

    for (int block = 0; block < 500; ++block)
    {
        // Randomise every parameter to a valid normalised value [0, 1]
        for (const auto& id : paramIDs)
        {
            if (auto* param = proc.apvts.getParameter (id))
                param->setValueNotifyingHost (rng.nextFloat());
        }

        fillPinkNoise (buffer, 0.5f, rng);
        proc.processBlock (buffer, midiBuffer);

        REQUIRE (allFinite (buffer));
    }
}

// ============================================================================
// test_buffer_size_variation
// Process blocks of varying sizes in sequence; verify no crash.
// ============================================================================
TEST_CASE("test_buffer_size_variation", "[ProcessorStress]")
{
    MLIMAudioProcessor proc;
    proc.prepareToPlay (kSampleRate, 4096); // prepare with max expected block size

    juce::Random rng (7);
    juce::MidiBuffer midiBuffer;

    const std::vector<int> blockSizes { 1, 32, 64, 128, 256, 512, 1024, 4096 };

    for (int size : blockSizes)
    {
        juce::AudioBuffer<float> buffer (kNumChannels, size);
        fillPinkNoise (buffer, 0.5f, rng);
        proc.processBlock (buffer, midiBuffer);

        REQUIRE (allFinite (buffer));
    }

    // Process the sequence twice to catch state-dependent bugs
    for (int size : blockSizes)
    {
        juce::AudioBuffer<float> buffer (kNumChannels, size);
        fillPinkNoise (buffer, 0.5f, rng);
        proc.processBlock (buffer, midiBuffer);

        REQUIRE (allFinite (buffer));
    }
}

// ============================================================================
// test_prepare_release_cycle
// Call prepareToPlay/releaseResources 10 times, then process; verify correct
// operation after repeated lifecycle transitions.
// ============================================================================
TEST_CASE("test_prepare_release_cycle", "[ProcessorStress]")
{
    MLIMAudioProcessor proc;

    for (int i = 0; i < 10; ++i)
    {
        proc.prepareToPlay (kSampleRate, kBlockSize);
        proc.releaseResources();
    }

    // Final prepare before processing
    proc.prepareToPlay (kSampleRate, kBlockSize);

    juce::Random rng (99);
    juce::AudioBuffer<float> buffer (kNumChannels, kBlockSize);
    juce::MidiBuffer midiBuffer;

    for (int block = 0; block < 50; ++block)
    {
        fillPinkNoise (buffer, 0.5f, rng);
        proc.processBlock (buffer, midiBuffer);
        REQUIRE (allFinite (buffer));
    }

    // Also verify ceiling is enforced after lifecycle churn
    const float ceilingLinear = std::pow (10.0f, -0.1f / 20.0f);
    const float tolerance     = 0.02f;

    for (int block = 0; block < 50; ++block)
    {
        fillPinkNoise (buffer, 4.0f, rng); // very hot input
        proc.processBlock (buffer, midiBuffer);

        if (block > 5) // allow limiter to engage
            REQUIRE (peakLevel (buffer) <= ceilingLinear + tolerance);
    }
}

// ============================================================================
// test_all_features_enabled
// Enable truePeak + oversampling(4x) + dcFilter + dither + hot input; process
// 100 blocks and verify output is within ceiling and all samples are finite.
// ============================================================================
TEST_CASE("test_all_features_enabled", "[ProcessorStress]")
{
    MLIMAudioProcessor proc;

    // Set all features on
    auto setParam = [&] (const juce::String& id, float value)
    {
        if (auto* p = proc.apvts.getParameter (id))
            p->setValueNotifyingHost (p->convertTo0to1 (value));
    };

    setParam (ParamID::inputGain,       12.0f);   // +12 dB input gain
    setParam (ParamID::outputCeiling,   -0.1f);   // -0.1 dBTP ceiling

    // Oversampling: 4x = index 2 in {Off,2x,4x,8x,16x,32x}
    if (auto* p = proc.apvts.getParameter (ParamID::oversamplingFactor))
        p->setValueNotifyingHost (p->convertTo0to1 (2.0f));

    // Boolean features
    if (auto* p = proc.apvts.getParameter (ParamID::truePeakEnabled))
        p->setValueNotifyingHost (1.0f);
    if (auto* p = proc.apvts.getParameter (ParamID::dcFilterEnabled))
        p->setValueNotifyingHost (1.0f);
    if (auto* p = proc.apvts.getParameter (ParamID::ditherEnabled))
        p->setValueNotifyingHost (1.0f);

    // Dither bit depth: 16 = index 0 in {16,18,20,22,24}
    if (auto* p = proc.apvts.getParameter (ParamID::ditherBitDepth))
        p->setValueNotifyingHost (0.0f);

    // Apply changes via prepareToPlay (oversampling reallocation is deferred)
    proc.prepareToPlay (kSampleRate, kBlockSize);

    const float ceilingLinear = std::pow (10.0f, -0.1f / 20.0f);
    const float tolerance     = 0.02f;

    juce::Random rng (123);
    juce::AudioBuffer<float> buffer (kNumChannels, kBlockSize);
    juce::MidiBuffer midiBuffer;

    for (int block = 0; block < 100; ++block)
    {
        fillPinkNoise (buffer, 0.5f, rng); // signal is boosted by +12 dB inside
        proc.processBlock (buffer, midiBuffer);
        REQUIRE (allFinite (buffer));

        if (block > 10) // allow limiter to engage
            REQUIRE (peakLevel (buffer) <= ceilingLinear + tolerance);
    }
}

// ============================================================================
// test_all_algorithms_process_cleanly
// Iterate through all 8 algorithms; process 50 blocks of pink noise at
// -6 dBFS each, verify output never exceeds ceiling and is all-finite.
// ============================================================================
TEST_CASE("test_all_algorithms_process_cleanly", "[ProcessorStress]")
{
    const float inputAmplitude = std::pow (10.0f, -6.0f / 20.0f); // -6 dBFS
    const float ceilingDb      = -0.1f;
    const float ceilingLinear  = std::pow (10.0f, ceilingDb / 20.0f);
    const float tolerance      = 0.1f; // 0.1 dB in linear headroom

    const int numAlgorithms = 8;

    for (int algoIdx = 0; algoIdx < numAlgorithms; ++algoIdx)
    {
        MLIMAudioProcessor proc;
        proc.prepareToPlay (kSampleRate, kBlockSize);

        // Set the algorithm by normalised index
        if (auto* p = proc.apvts.getParameter (ParamID::algorithm))
            p->setValueNotifyingHost (p->convertTo0to1 (static_cast<float> (algoIdx)));

        juce::Random rng (algoIdx * 31 + 7); // reproducible per algorithm
        juce::AudioBuffer<float> buffer (kNumChannels, kBlockSize);
        juce::MidiBuffer midiBuffer;

        for (int block = 0; block < 50; ++block)
        {
            fillPinkNoise (buffer, inputAmplitude, rng);
            proc.processBlock (buffer, midiBuffer);
            REQUIRE (allFinite (buffer));

            if (block > 5) // allow limiter to engage
                REQUIRE (peakLevel (buffer) <= ceilingLinear + tolerance);
        }
    }
}

// ============================================================================
// test_sample_rate_switch
// Prepare at 44100, process some blocks, then 96000, then 48000; verify no
// crash and output remains finite throughout.
// ============================================================================
TEST_CASE("test_sample_rate_switch", "[ProcessorStress]")
{
    MLIMAudioProcessor proc;
    juce::Random rng (77);
    juce::MidiBuffer midiBuffer;

    const std::vector<double> sampleRates { 44100.0, 96000.0, 48000.0 };

    for (double sr : sampleRates)
    {
        proc.prepareToPlay (sr, kBlockSize);

        juce::AudioBuffer<float> buffer (kNumChannels, kBlockSize);

        for (int block = 0; block < 30; ++block)
        {
            fillPinkNoise (buffer, 0.5f, rng);
            proc.processBlock (buffer, midiBuffer);
            REQUIRE (allFinite (buffer));
        }
    }
}
