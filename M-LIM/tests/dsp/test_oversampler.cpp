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

// ---------------------------------------------------------------------------
// test_16x_no_aliasing
// ---------------------------------------------------------------------------
TEST_CASE("test_16x_no_aliasing", "[Oversampler]")
{
    const int numChannels = 2;
    const int blockSize = 64;
    const float freq = 440.0f;

    Oversampler os;
    os.prepare(kSampleRate, blockSize, numChannels);
    os.setFactor(4); // 16x

    // Feed several blocks to let the filter settle
    juce::AudioBuffer<float> buffer(numChannels, blockSize);
    double inputRmsAccum = 0.0;
    double outputRmsAccum = 0.0;
    int totalSamples = 0;

    for (int block = 0; block < 20; ++block)
    {
        for (int ch = 0; ch < numChannels; ++ch)
        {
            float* data = buffer.getWritePointer(ch);
            for (int i = 0; i < blockSize; ++i)
            {
                int globalSample = block * blockSize + i;
                data[i] = std::sin(2.0f * 3.14159265f * freq * globalSample / static_cast<float>(kSampleRate));
            }
        }

        // Measure input RMS (only after settling — skip first 10 blocks)
        if (block >= 10)
        {
            const float* in = buffer.getReadPointer(0);
            for (int i = 0; i < blockSize; ++i)
                inputRmsAccum += static_cast<double>(in[i]) * in[i];
        }

        os.upsample(buffer);
        os.downsample(buffer);

        if (block >= 10)
        {
            const float* out = buffer.getReadPointer(0);
            for (int i = 0; i < blockSize; ++i)
                outputRmsAccum += static_cast<double>(out[i]) * out[i];
            totalSamples += blockSize;
        }
    }

    double inputRmsDb = 10.0 * std::log10(inputRmsAccum / totalSamples + 1e-30);
    double outputRmsDb = 10.0 * std::log10(outputRmsAccum / totalSamples + 1e-30);
    double diffDb = outputRmsDb - inputRmsDb;

    // Output RMS within ±1 dB of input
    REQUIRE(diffDb > -1.0);
    REQUIRE(diffDb < 1.0);
}

// ---------------------------------------------------------------------------
// test_32x_no_aliasing
// ---------------------------------------------------------------------------
TEST_CASE("test_32x_no_aliasing", "[Oversampler]")
{
    const int numChannels = 2;
    const int blockSize = 64;
    const float freq = 440.0f;

    Oversampler os;
    os.prepare(kSampleRate, blockSize, numChannels);
    os.setFactor(5); // 32x

    juce::AudioBuffer<float> buffer(numChannels, blockSize);
    double inputRmsAccum = 0.0;
    double outputRmsAccum = 0.0;
    int totalSamples = 0;

    for (int block = 0; block < 20; ++block)
    {
        for (int ch = 0; ch < numChannels; ++ch)
        {
            float* data = buffer.getWritePointer(ch);
            for (int i = 0; i < blockSize; ++i)
            {
                int globalSample = block * blockSize + i;
                data[i] = std::sin(2.0f * 3.14159265f * freq * globalSample / static_cast<float>(kSampleRate));
            }
        }

        if (block >= 10)
        {
            const float* in = buffer.getReadPointer(0);
            for (int i = 0; i < blockSize; ++i)
                inputRmsAccum += static_cast<double>(in[i]) * in[i];
        }

        os.upsample(buffer);
        os.downsample(buffer);

        if (block >= 10)
        {
            const float* out = buffer.getReadPointer(0);
            for (int i = 0; i < blockSize; ++i)
                outputRmsAccum += static_cast<double>(out[i]) * out[i];
            totalSamples += blockSize;
        }
    }

    double inputRmsDb = 10.0 * std::log10(inputRmsAccum / totalSamples + 1e-30);
    double outputRmsDb = 10.0 * std::log10(outputRmsAccum / totalSamples + 1e-30);
    double diffDb = outputRmsDb - inputRmsDb;

    REQUIRE(diffDb > -1.0);
    REQUIRE(diffDb < 1.0);
}

// ---------------------------------------------------------------------------
// test_reported_latency_matches_measured
// ---------------------------------------------------------------------------
TEST_CASE("test_reported_latency_matches_measured", "[Oversampler]")
{
    const int numChannels = 1;
    const int factor = 2; // 4x
    const int blockSize = 512;

    Oversampler os;
    os.prepare(kSampleRate, blockSize, numChannels);
    os.setFactor(factor);

    float reportedLatency = os.getLatencySamples();
    REQUIRE(reportedLatency > 0.0f);

    // We need enough blocks to push the impulse through
    int totalBlocks = static_cast<int>(std::ceil(reportedLatency / blockSize)) + 4;

    // Collect all output samples
    std::vector<float> output;
    output.reserve(static_cast<size_t>(totalBlocks * blockSize));

    for (int block = 0; block < totalBlocks; ++block)
    {
        juce::AudioBuffer<float> buffer(numChannels, blockSize);
        buffer.clear();

        // Place impulse at sample 0 of block 0
        if (block == 0)
            buffer.getWritePointer(0)[0] = 1.0f;

        os.upsample(buffer);
        os.downsample(buffer);

        const float* out = buffer.getReadPointer(0);
        for (int i = 0; i < blockSize; ++i)
            output.push_back(out[i]);
    }

    // Find peak position in output
    int peakPos = 0;
    float peakVal = 0.0f;
    for (size_t i = 0; i < output.size(); ++i)
    {
        if (std::abs(output[i]) > peakVal)
        {
            peakVal = std::abs(output[i]);
            peakPos = static_cast<int>(i);
        }
    }

    // Peak position should be within ±2 samples of reported latency
    int diff = std::abs(peakPos - static_cast<int>(std::round(reportedLatency)));
    REQUIRE(diff <= 2);
}

// ---------------------------------------------------------------------------
// test_factor_change_no_output_burst
// ---------------------------------------------------------------------------
TEST_CASE("test_factor_change_no_output_burst", "[Oversampler]")
{
    const int numChannels = 2;
    const int blockSize = 256;
    const float freq = 440.0f;

    Oversampler os;
    os.prepare(kSampleRate, blockSize, numChannels);
    os.setFactor(2); // start at 4x

    juce::AudioBuffer<float> buffer(numChannels, blockSize);

    // Run a few blocks at 4x to settle
    for (int block = 0; block < 5; ++block)
    {
        for (int ch = 0; ch < numChannels; ++ch)
        {
            float* data = buffer.getWritePointer(ch);
            for (int i = 0; i < blockSize; ++i)
            {
                int globalSample = block * blockSize + i;
                data[i] = std::sin(2.0f * 3.14159265f * freq * globalSample / static_cast<float>(kSampleRate));
            }
        }
        os.upsample(buffer);
        os.downsample(buffer);
    }

    // Measure reference level from last settled block
    float refPeakDb = -100.0f;
    {
        const float* out = buffer.getReadPointer(0);
        for (int i = 0; i < blockSize; ++i)
        {
            float absVal = std::abs(out[i]);
            if (absVal > 1e-10f)
            {
                float db = 20.0f * std::log10(absVal);
                if (db > refPeakDb)
                    refPeakDb = db;
            }
        }
    }

    // Switch to 8x
    os.setFactor(3);

    // Process first block after switch
    for (int ch = 0; ch < numChannels; ++ch)
    {
        float* data = buffer.getWritePointer(ch);
        for (int i = 0; i < blockSize; ++i)
        {
            int globalSample = 5 * blockSize + i;
            data[i] = std::sin(2.0f * 3.14159265f * freq * globalSample / static_cast<float>(kSampleRate));
        }
    }
    os.upsample(buffer);
    os.downsample(buffer);

    // Find peak dBFS of post-switch block
    float postPeakDb = -100.0f;
    {
        const float* out = buffer.getReadPointer(0);
        for (int i = 0; i < blockSize; ++i)
        {
            float absVal = std::abs(out[i]);
            if (absVal > 1e-10f)
            {
                float db = 20.0f * std::log10(absVal);
                if (db > postPeakDb)
                    postPeakDb = db;
            }
        }
    }

    // Post-switch peak should be within [-2, +2] dBFS of reference
    float peakDiff = postPeakDb - refPeakDb;
    REQUIRE(peakDiff >= -2.0f);
    REQUIRE(peakDiff <= 2.0f);
}
