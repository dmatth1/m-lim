/**
 * test_dsp_components.cpp — DSP Component Integration Checkpoint (Task 039)
 *
 * Verifies that all DSP component headers compile together without conflicts,
 * that each component can be prepared and run, and that a basic DSP chain
 * (oversample → transient limit → leveling limit → sidechain filter → DC filter → dither)
 * can be wired together and process audio without crashing.
 */
#include "catch2/catch_amalgamated.hpp"

// Include all DSP headers together to verify no conflicts
#include "dsp/TransientLimiter.h"
#include "dsp/LevelingLimiter.h"
#include "dsp/Oversampler.h"
#include "dsp/TruePeakDetector.h"
#include "dsp/SidechainFilter.h"
#include "dsp/DCFilter.h"
#include "dsp/Dither.h"
#include "dsp/LoudnessMeter.h"
#include "dsp/LimiterAlgorithm.h"

#include <juce_audio_basics/juce_audio_basics.h>
#include <juce_dsp/juce_dsp.h>
#include <cmath>
#include <vector>

static constexpr double kSampleRate = 44100.0;
static constexpr int    kBlockSize  = 512;
static constexpr int    kNumChannels = 2;

// ============================================================================
// test_all_dsp_headers_compile
// If this test file compiles, all headers are conflict-free.
// ============================================================================
TEST_CASE("test_all_dsp_headers_compile", "[DSPIntegration]")
{
    // If we reach here, all DSP headers compiled without conflicts.
    // Verify the algorithm enum has the expected values.
    REQUIRE(static_cast<int>(LimiterAlgorithm::Transparent) == 0);
    REQUIRE(static_cast<int>(LimiterAlgorithm::Modern)      == 7);

    // Verify AlgorithmParams struct is accessible
    AlgorithmParams params = getAlgorithmParams(LimiterAlgorithm::Transparent);
    REQUIRE(params.kneeWidth > 0.0f);
    REQUIRE(params.adaptiveRelease == true);
}

// ============================================================================
// test_all_dsp_prepare
// Instantiate every DSP component and call prepare() without crashing.
// ============================================================================
TEST_CASE("test_all_dsp_prepare", "[DSPIntegration]")
{
    SECTION("TransientLimiter prepare")
    {
        TransientLimiter tl;
        tl.prepare(kSampleRate, kBlockSize, kNumChannels);
        // Should not throw or crash
    }

    SECTION("LevelingLimiter prepare")
    {
        LevelingLimiter ll;
        ll.prepare(kSampleRate, kBlockSize, kNumChannels);
    }

    SECTION("Oversampler prepare")
    {
        Oversampler os;
        os.prepare(kSampleRate, kBlockSize, kNumChannels);
    }

    SECTION("TruePeakDetector prepare")
    {
        TruePeakDetector tp;
        tp.prepare(kSampleRate);
    }

    SECTION("SidechainFilter prepare")
    {
        SidechainFilter sf;
        sf.prepare(kSampleRate, kBlockSize);
    }

    SECTION("DCFilter prepare")
    {
        DCFilter dcf;
        dcf.prepare(kSampleRate);
    }

    SECTION("Dither prepare")
    {
        Dither d;
        d.prepare(kSampleRate);
    }

    SECTION("LoudnessMeter prepare")
    {
        LoudnessMeter meter;
        meter.prepare(kSampleRate, kNumChannels);
    }
}

// ============================================================================
// test_dsp_chain_basic
// Wire components in the order used by LimiterEngine:
//   Input → Oversampler (upsample) → TransientLimiter → LevelingLimiter
//         → Oversampler (downsample) → DCFilter → Dither → Output
// SidechainFilter and LoudnessMeter run in parallel on a copy.
// Process one block of a 1kHz sine and verify output is finite.
// ============================================================================
TEST_CASE("test_dsp_chain_basic", "[DSPIntegration]")
{
    // Instantiate all components
    Oversampler      oversampler;
    TransientLimiter transient;
    LevelingLimiter  leveling;
    SidechainFilter  sidechain;
    DCFilter         dcFilter;
    Dither           dither;
    LoudnessMeter    loudness;
    TruePeakDetector truePeak;

    // Prepare all at 44100 Hz, 512-sample blocks, stereo
    oversampler.prepare(kSampleRate, kBlockSize, kNumChannels);
    transient.prepare(kSampleRate, kBlockSize, kNumChannels);
    leveling.prepare(kSampleRate, kBlockSize, kNumChannels);
    sidechain.prepare(kSampleRate, kBlockSize);
    dcFilter.prepare(kSampleRate);
    dither.prepare(kSampleRate);
    loudness.prepare(kSampleRate, kNumChannels);
    truePeak.prepare(kSampleRate);

    // Set algorithm params on limiters
    AlgorithmParams params = getAlgorithmParams(LimiterAlgorithm::Transparent);
    transient.setAlgorithmParams(params);

    // Generate 1kHz test signal at 0 dBFS (loud enough to trigger limiting)
    static constexpr double kTwoPi = 6.283185307179586;
    juce::AudioBuffer<float> mainBuf(kNumChannels, kBlockSize);
    for (int ch = 0; ch < kNumChannels; ++ch)
    {
        float* data = mainBuf.getWritePointer(ch);
        for (int i = 0; i < kBlockSize; ++i)
            data[i] = 0.9f * static_cast<float>(
                          std::sin(kTwoPi * 1000.0 * i / kSampleRate));
    }

    // --- Step 1: Sidechain copy for parallel detection ---
    juce::AudioBuffer<float> sideBuf(mainBuf);
    sidechain.process(sideBuf);

    // Build sidechain float** for limiters
    std::vector<const float*> sidePtrs(kNumChannels);
    for (int ch = 0; ch < kNumChannels; ++ch)
        sidePtrs[ch] = sideBuf.getReadPointer(ch);

    // --- Step 2: Oversample up ---
    juce::dsp::AudioBlock<float> upBlock = oversampler.upsample(mainBuf);

    // Extract float** pointers from the upsampled block for limiter processing
    const int upSamples   = static_cast<int>(upBlock.getNumSamples());
    const int upChannels  = static_cast<int>(upBlock.getNumChannels());
    std::vector<float*> upPtrs(upChannels);
    for (int ch = 0; ch < upChannels; ++ch)
        upPtrs[ch] = upBlock.getChannelPointer(ch);

    // --- Step 3: TransientLimiter ---
    // (factor==0 means 1x, so upSamples == kBlockSize; both limiters use main pointers)
    transient.process(upPtrs.data(), upChannels, upSamples, sidePtrs.data());

    // --- Step 4: LevelingLimiter ---
    leveling.process(upPtrs.data(), upChannels, upSamples);

    // --- Step 5: Downsample ---
    oversampler.downsample(mainBuf);

    // --- Step 6: DC filter per channel ---
    for (int ch = 0; ch < kNumChannels; ++ch)
        dcFilter.process(mainBuf.getWritePointer(ch), kBlockSize);

    // --- Step 7: Dither per channel ---
    for (int ch = 0; ch < kNumChannels; ++ch)
        dither.process(mainBuf.getWritePointer(ch), kBlockSize);

    // --- Step 8: Loudness meter (parallel) ---
    loudness.processBlock(mainBuf);

    // --- Step 9: True peak detection (parallel) ---
    for (int ch = 0; ch < kNumChannels; ++ch)
        truePeak.processBlock(mainBuf.getReadPointer(ch), kBlockSize);

    // Verify output is valid (finite, no NaN/Inf)
    for (int ch = 0; ch < kNumChannels; ++ch)
    {
        const float* data = mainBuf.getReadPointer(ch);
        for (int i = 0; i < kBlockSize; ++i)
            REQUIRE(std::isfinite(data[i]));
    }

    // True peak should be non-negative
    REQUIRE(truePeak.getPeak() >= 0.0f);
}

// ============================================================================
// test_dsp_chain_with_oversampling
// Same wiring as above but with 2x oversampling enabled.
// ============================================================================
TEST_CASE("test_dsp_chain_with_oversampling", "[DSPIntegration]")
{
    Oversampler      oversampler;
    TransientLimiter transient;
    LevelingLimiter  leveling;
    DCFilter         dcFilter;
    Dither           dither;

    oversampler.setFactor(1); // 2x
    oversampler.prepare(kSampleRate, kBlockSize, kNumChannels);
    transient.prepare(kSampleRate * 2, kBlockSize * 2, kNumChannels);
    leveling.prepare(kSampleRate * 2, kBlockSize * 2, kNumChannels);
    dcFilter.prepare(kSampleRate);
    dither.prepare(kSampleRate);

    // Generate test signal
    static constexpr double kTwoPi = 6.283185307179586;
    juce::AudioBuffer<float> mainBuf(kNumChannels, kBlockSize);
    for (int ch = 0; ch < kNumChannels; ++ch)
    {
        float* data = mainBuf.getWritePointer(ch);
        for (int i = 0; i < kBlockSize; ++i)
            data[i] = 0.9f * static_cast<float>(
                          std::sin(kTwoPi * 1000.0 * i / kSampleRate));
    }

    // Upsample
    juce::dsp::AudioBlock<float> upBlock = oversampler.upsample(mainBuf);
    const int upSamples  = static_cast<int>(upBlock.getNumSamples());
    const int upChannels = static_cast<int>(upBlock.getNumChannels());
    std::vector<float*> upPtrs(upChannels);
    for (int ch = 0; ch < upChannels; ++ch)
        upPtrs[ch] = upBlock.getChannelPointer(ch);

    transient.process(upPtrs.data(), upChannels, upSamples, nullptr);
    leveling.process(upPtrs.data(), upChannels, upSamples);

    oversampler.downsample(mainBuf);

    for (int ch = 0; ch < kNumChannels; ++ch)
        dcFilter.process(mainBuf.getWritePointer(ch), kBlockSize);

    // Verify output is finite
    for (int ch = 0; ch < kNumChannels; ++ch)
    {
        const float* data = mainBuf.getReadPointer(ch);
        for (int i = 0; i < kBlockSize; ++i)
            REQUIRE(std::isfinite(data[i]));
    }
}
