#include "catch2/catch_amalgamated.hpp"
#include "dsp/Oversampler.h"
#include <juce_audio_basics/juce_audio_basics.h>
#include <cmath>
#include <vector>

static constexpr double kSampleRate = 44100.0;
static constexpr int    kBlockSize  = 512;

// ---------------------------------------------------------------------------
// test_2x_upsample_doubles_samples
// ---------------------------------------------------------------------------
TEST_CASE("test_2x_upsample_doubles_samples", "[Oversampler]")
{
    Oversampler os;
    const int numChannels = 2;
    os.prepare(kSampleRate, kBlockSize, numChannels);
    os.setFactor(1); // 2x oversampling

    juce::AudioBuffer<float> buffer(numChannels, kBlockSize);
    for (int ch = 0; ch < numChannels; ++ch)
    {
        float* data = buffer.getWritePointer(ch);
        for (int i = 0; i < kBlockSize; ++i)
            data[i] = std::sin(2.0f * 3.14159265f * 440.0f * i / static_cast<float>(kSampleRate));
    }

    auto upBlock = os.upsample(buffer);
    REQUIRE(static_cast<int>(upBlock.getNumSamples()) == kBlockSize * 2);
}

// ---------------------------------------------------------------------------
// test_downsample_restores_length
// ---------------------------------------------------------------------------
TEST_CASE("test_downsample_restores_length", "[Oversampler]")
{
    Oversampler os;
    const int numChannels = 2;
    os.prepare(kSampleRate, kBlockSize, numChannels);
    os.setFactor(1); // 2x oversampling

    juce::AudioBuffer<float> buffer(numChannels, kBlockSize);
    buffer.clear();

    // Upsample then immediately downsample
    os.upsample(buffer);
    os.downsample(buffer);

    // Original buffer dimensions should be unchanged
    REQUIRE(buffer.getNumSamples() == kBlockSize);
    REQUIRE(buffer.getNumChannels() == numChannels);
}

// ---------------------------------------------------------------------------
// test_passthrough_when_off
// ---------------------------------------------------------------------------
TEST_CASE("test_passthrough_when_off", "[Oversampler]")
{
    Oversampler os;
    const int numChannels = 1;
    os.prepare(kSampleRate, kBlockSize, numChannels);
    os.setFactor(0); // off — 1x passthrough

    juce::AudioBuffer<float> buffer(numChannels, kBlockSize);
    float* data = buffer.getWritePointer(0);
    for (int i = 0; i < kBlockSize; ++i)
        data[i] = static_cast<float>(i) / static_cast<float>(kBlockSize);

    // Store original values for comparison
    std::vector<float> original(data, data + kBlockSize);

    // upsample returns a block wrapping the buffer; downsample is a no-op
    auto block = os.upsample(buffer);
    os.downsample(buffer);

    // Returned block should cover the same number of samples
    REQUIRE(static_cast<int>(block.getNumSamples()) == kBlockSize);

    // Buffer data should be unmodified
    const float* out = buffer.getReadPointer(0);
    for (int i = 0; i < kBlockSize; ++i)
        REQUIRE(out[i] == Catch::Approx(original[i]).margin(1e-6f));
}

// ---------------------------------------------------------------------------
// test_deferred_factor_change
// Calling requestFactor() during processing doesn't crash; rebuild happens
// only when commitRebuild() is called from a non-RT context.
// ---------------------------------------------------------------------------
TEST_CASE("test_deferred_factor_change", "[Oversampler]")
{
    Oversampler os;
    const int numChannels = 2;
    os.prepare(kSampleRate, kBlockSize, numChannels);
    os.setFactor(1); // 2x oversampling

    // Simulate "requesting" a factor change without immediate rebuild
    os.requestFactor(2); // ask for 4x oversampling

    // needsRebuild() must return true since the factor hasn't been applied yet
    REQUIRE(os.needsRebuild() == true);
    REQUIRE(os.getFactor() == 1); // still 2x until commitRebuild

    // Simulate a few "audio thread" processing cycles — no crash
    juce::AudioBuffer<float> buffer(numChannels, kBlockSize);
    buffer.clear();
    for (int i = 0; i < 5; ++i)
    {
        os.upsample(buffer);
        os.downsample(buffer);
    }

    // Now commit the rebuild from "non-RT" context
    os.commitRebuild();

    REQUIRE(os.needsRebuild() == false);
    REQUIRE(os.getFactor() == 2); // now 4x

    // Verify the rebuilt oversampler actually upsamples by 4x
    auto upBlock = os.upsample(buffer);
    REQUIRE(static_cast<int>(upBlock.getNumSamples()) == kBlockSize * 4);
    os.downsample(buffer);
}

// ---------------------------------------------------------------------------
// test_latency_reporting
// ---------------------------------------------------------------------------
TEST_CASE("test_latency_reporting", "[Oversampler]")
{
    Oversampler os;
    os.prepare(kSampleRate, kBlockSize, 2);

    // Factor 0 (off) — no latency
    os.setFactor(0);
    REQUIRE(os.getLatencySamples() == Catch::Approx(0.0f));

    // Factor 1 (2x) — IIR filter introduces latency
    os.setFactor(1);
    REQUIRE(os.getLatencySamples() > 0.0f);

    // Factor 2 (4x) — should also have latency
    os.setFactor(2);
    REQUIRE(os.getLatencySamples() > 0.0f);

    // Turning off again should report zero latency
    os.setFactor(0);
    REQUIRE(os.getLatencySamples() == Catch::Approx(0.0f));
}
