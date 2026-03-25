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

// ---------------------------------------------------------------------------
// test_all_factors_upsample_size
// ---------------------------------------------------------------------------
TEST_CASE("test_all_factors_upsample_size", "[Oversampler]")
{
    // For high factors, use a small block size to avoid excessive memory/time
    const int numChannels = 2;

    for (int factor = 0; factor <= 5; ++factor)
    {
        const int blockSize = (factor >= 4) ? 64 : 512;
        const int expectedUpSize = blockSize * (1 << factor);

        Oversampler os;
        os.prepare(kSampleRate, blockSize, numChannels);
        os.setFactor(factor);

        juce::AudioBuffer<float> buffer(numChannels, blockSize);
        buffer.clear();

        auto upBlock = os.upsample(buffer);
        REQUIRE(static_cast<int>(upBlock.getNumSamples()) == expectedUpSize);
    }
}

// ---------------------------------------------------------------------------
// test_output_finite_after_cycle
// ---------------------------------------------------------------------------
TEST_CASE("test_output_finite_after_cycle", "[Oversampler]")
{
    const int numChannels = 2;

    for (int factor = 0; factor <= 5; ++factor)
    {
        const int blockSize = (factor >= 4) ? 64 : 512;

        Oversampler os;
        os.prepare(kSampleRate, blockSize, numChannels);
        os.setFactor(factor);

        juce::AudioBuffer<float> buffer(numChannels, blockSize);
        for (int ch = 0; ch < numChannels; ++ch)
        {
            float* data = buffer.getWritePointer(ch);
            for (int i = 0; i < blockSize; ++i)
                data[i] = std::sin(2.0f * 3.14159265f * 440.0f * i / static_cast<float>(kSampleRate));
        }

        os.upsample(buffer);
        os.downsample(buffer);

        // All output samples must be finite (no NaN/Inf)
        for (int ch = 0; ch < numChannels; ++ch)
        {
            const float* out = buffer.getReadPointer(ch);
            for (int i = 0; i < blockSize; ++i)
                REQUIRE(std::isfinite(out[i]));
        }
    }
}

// ---------------------------------------------------------------------------
// test_latency_monotonic
// ---------------------------------------------------------------------------
TEST_CASE("test_latency_monotonic", "[Oversampler]")
{
    Oversampler os;
    os.prepare(kSampleRate, kBlockSize, 2);

    os.setFactor(0);
    float lat0 = os.getLatencySamples();

    os.setFactor(1);
    float lat1 = os.getLatencySamples();

    os.setFactor(2);
    float lat2 = os.getLatencySamples();

    os.setFactor(3);
    float lat3 = os.getLatencySamples();

    REQUIRE(lat0 == Catch::Approx(0.0f));
    REQUIRE(lat1 > lat0);
    REQUIRE(lat2 > lat1);
    REQUIRE(lat3 > lat2);
}

// ---------------------------------------------------------------------------
// test_repeated_cycles_stable
// ---------------------------------------------------------------------------
TEST_CASE("test_repeated_cycles_stable", "[Oversampler]")
{
    const int numChannels = 2;
    const int factor = 2; // 4x

    Oversampler os;
    os.prepare(kSampleRate, kBlockSize, numChannels);
    os.setFactor(factor);

    juce::AudioBuffer<float> buffer(numChannels, kBlockSize);

    float prevMaxMag = -1.0f;

    for (int cycle = 0; cycle < 10; ++cycle)
    {
        // Fill with a unit-amplitude sine
        for (int ch = 0; ch < numChannels; ++ch)
        {
            float* data = buffer.getWritePointer(ch);
            for (int i = 0; i < kBlockSize; ++i)
                data[i] = std::sin(2.0f * 3.14159265f * 440.0f * i / static_cast<float>(kSampleRate));
        }

        os.upsample(buffer);
        os.downsample(buffer);

        float maxMag = 0.0f;
        for (int ch = 0; ch < numChannels; ++ch)
        {
            const float* out = buffer.getReadPointer(ch);
            for (int i = 0; i < kBlockSize; ++i)
                maxMag = std::max(maxMag, std::abs(out[i]));
        }

        // Output must be finite and bounded (not diverging)
        REQUIRE(std::isfinite(maxMag));
        REQUIRE(maxMag <= 2.0f); // generous bound — should be near 1.0

        prevMaxMag = maxMag;
    }
    (void)prevMaxMag;
}

// ---------------------------------------------------------------------------
// test_small_block_size_no_crash
// ---------------------------------------------------------------------------
TEST_CASE("test_small_block_size_no_crash", "[Oversampler]")
{
    const int numChannels = 2;
    const int factor = 2; // 4x

    for (int blockSize : {1, 4, 16})
    {
        Oversampler os;
        os.prepare(kSampleRate, blockSize, numChannels);
        os.setFactor(factor);

        juce::AudioBuffer<float> buffer(numChannels, blockSize);
        buffer.clear();

        auto upBlock = os.upsample(buffer);
        REQUIRE(static_cast<int>(upBlock.getNumSamples()) == blockSize * (1 << factor));

        os.downsample(buffer);
        REQUIRE(buffer.getNumSamples() == blockSize);
    }
}

// ---------------------------------------------------------------------------
// test_set_same_factor_idempotent
// ---------------------------------------------------------------------------
TEST_CASE("test_set_same_factor_idempotent", "[Oversampler]")
{
    const int numChannels = 2;

    Oversampler os;
    os.prepare(kSampleRate, kBlockSize, numChannels);

    // Set factor 1 twice — second call should be a no-op, no crash
    os.setFactor(1);
    os.setFactor(1);

    juce::AudioBuffer<float> buffer(numChannels, kBlockSize);
    buffer.clear();

    auto upBlock = os.upsample(buffer);
    REQUIRE(static_cast<int>(upBlock.getNumSamples()) == kBlockSize * 2);

    os.downsample(buffer);
    REQUIRE(buffer.getNumSamples() == kBlockSize);
}
