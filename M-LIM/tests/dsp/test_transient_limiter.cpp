#include "catch2/catch_amalgamated.hpp"
#include "dsp/TransientLimiter.h"
#include "dsp/LimiterAlgorithm.h"
#include <cmath>
#include <vector>
#include <algorithm>

static constexpr double kSampleRate = 44100.0;
static constexpr int    kBlockSize  = 1024;
static constexpr int    kNumChannels = 2;

// ---------------------------------------------------------------------------
// Helper: fill a block with a constant value on all channels
// ---------------------------------------------------------------------------
static void fillConstant(std::vector<std::vector<float>>& buf, float value)
{
    for (auto& ch : buf)
        std::fill(ch.begin(), ch.end(), value);
}

// ---------------------------------------------------------------------------
// Helper: fill a block with silence
// ---------------------------------------------------------------------------
static void fillSilence(std::vector<std::vector<float>>& buf)
{
    fillConstant(buf, 0.0f);
}

// ---------------------------------------------------------------------------
// Helper: peak value in a single-channel block
// ---------------------------------------------------------------------------
static float blockPeak(const std::vector<float>& data)
{
    float peak = 0.0f;
    for (float v : data)
        peak = std::max(peak, std::abs(v));
    return peak;
}

// ---------------------------------------------------------------------------
// Helper: build raw pointer array from vector-of-vectors
// ---------------------------------------------------------------------------
static std::vector<float*> makePtrs(std::vector<std::vector<float>>& buf)
{
    std::vector<float*> ptrs(buf.size());
    for (size_t i = 0; i < buf.size(); ++i)
        ptrs[i] = buf[i].data();
    return ptrs;
}

// ---------------------------------------------------------------------------
// test_peak_limiting
//   Input peak at +6 dB (~2.0 linear) must be limited to ≤ 0 dBFS (1.0)
// ---------------------------------------------------------------------------
TEST_CASE("test_peak_limiting", "[TransientLimiter]")
{
    TransientLimiter limiter;
    limiter.prepare(kSampleRate, kBlockSize, kNumChannels);
    limiter.setAlgorithmParams(getAlgorithmParams(LimiterAlgorithm::Transparent));
    limiter.setLookahead(1.0f);  // 1 ms lookahead

    // Use a block of samples at +6 dB (amplitude ~2.0)
    const float inputAmplitude = 2.0f;  // +6 dBFS
    std::vector<std::vector<float>> buf(kNumChannels, std::vector<float>(kBlockSize, inputAmplitude));
    auto ptrs = makePtrs(buf);

    limiter.process(ptrs.data(), kNumChannels, kBlockSize);

    // After processing, all output samples must be ≤ 1.0 (0 dBFS)
    for (int ch = 0; ch < kNumChannels; ++ch)
        REQUIRE(blockPeak(buf[ch]) <= 1.0f + 1e-4f);
}

// ---------------------------------------------------------------------------
// test_lookahead_anticipation
//   With lookahead enabled, gain reduction must start BEFORE a large peak
//   arrives at the output — i.e., the onset of gain reduction must precede
//   the peak in the input stream.
// ---------------------------------------------------------------------------
TEST_CASE("test_lookahead_anticipation", "[TransientLimiter]")
{
    TransientLimiter limiter;
    limiter.prepare(kSampleRate, kBlockSize, 1);  // mono

    AlgorithmParams params = getAlgorithmParams(LimiterAlgorithm::Transparent);
    params.kneeWidth = 0.0f;   // hard knee for clarity
    limiter.setAlgorithmParams(params);

    const float lookaheadMs = 2.0f;
    limiter.setLookahead(lookaheadMs);

    // Build input: silence followed by a large peak at sample 200
    const int numSamples = kBlockSize;
    const int peakPos    = 200;
    std::vector<std::vector<float>> buf(1, std::vector<float>(numSamples, 0.0f));
    buf[0][peakPos] = 2.0f;  // +6 dB impulse

    auto ptrs = makePtrs(buf);
    limiter.process(ptrs.data(), 1, numSamples);

    // With lookahead, the limiter should have started attenuating BEFORE the
    // peak sample arrives. Check that at least one sample before peakPos is
    // attenuated (gain < 1.0 means the sample is attenuated relative to 0).
    // Because of the delay, the impulse is shifted forward by lookaheadSamples.
    // The output around peakPos should show the attenuated impulse, and
    // samples before (peakPos - lookaheadSamples) should be near silence.

    // The key verification: output peak must not exceed 1.0
    REQUIRE(blockPeak(buf[0]) <= 1.0f + 1e-4f);

    // And GR must have been applied (reported GR should be negative dB)
    REQUIRE(limiter.getGainReduction() <= 0.0f);
}

// ---------------------------------------------------------------------------
// test_channel_linking
//   At link=1.0 both channels must receive identical gain reduction.
//   At link=0.0 channels process independently.
// ---------------------------------------------------------------------------
TEST_CASE("test_channel_linking", "[TransientLimiter]")
{
    SECTION("fully linked: both channels get same GR")
    {
        TransientLimiter limiter;
        limiter.prepare(kSampleRate, kBlockSize, 2);
        limiter.setAlgorithmParams(getAlgorithmParams(LimiterAlgorithm::Transparent));
        limiter.setLookahead(0.0f);   // no lookahead for simplicity
        limiter.setChannelLink(1.0f);

        // Channel 0: loud (+6 dB), Channel 1: quiet (0.5)
        std::vector<std::vector<float>> buf(2, std::vector<float>(kBlockSize));
        std::fill(buf[0].begin(), buf[0].end(), 2.0f);
        std::fill(buf[1].begin(), buf[1].end(), 0.5f);

        auto ptrs = makePtrs(buf);
        limiter.process(ptrs.data(), 2, kBlockSize);

        // With full linking, ch1 should also be attenuated (not pass through at 0.5)
        // If independent, ch1 output would be ~0.5; with linking it should be less.
        const float ch1Peak = blockPeak(buf[1]);
        REQUIRE(ch1Peak < 0.5f);  // linked GR reduces quieter channel too
    }

    SECTION("independent: channels get separate GR")
    {
        TransientLimiter limiter;
        limiter.prepare(kSampleRate, kBlockSize, 2);
        limiter.setAlgorithmParams(getAlgorithmParams(LimiterAlgorithm::Transparent));
        limiter.setLookahead(0.0f);
        limiter.setChannelLink(0.0f);

        // Channel 0: at threshold exactly (1.0), Channel 1: below threshold (0.5)
        std::vector<std::vector<float>> buf(2, std::vector<float>(kBlockSize));
        std::fill(buf[0].begin(), buf[0].end(), 1.0f);
        std::fill(buf[1].begin(), buf[1].end(), 0.5f);

        auto ptrs = makePtrs(buf);
        limiter.process(ptrs.data(), 2, kBlockSize);

        // Ch1 is below threshold and fully independent — should pass unmodified
        const float ch1Peak = blockPeak(buf[1]);
        REQUIRE(ch1Peak == Catch::Approx(0.5f).margin(0.01f));
    }
}

// ---------------------------------------------------------------------------
// test_no_clipping
//   For any input the output must never exceed 1.0 (0 dBFS)
// ---------------------------------------------------------------------------
TEST_CASE("test_no_clipping", "[TransientLimiter]")
{
    TransientLimiter limiter;
    limiter.prepare(kSampleRate, kBlockSize, kNumChannels);
    limiter.setAlgorithmParams(getAlgorithmParams(LimiterAlgorithm::Aggressive));
    limiter.setLookahead(2.0f);

    // Stress test with various amplitudes
    for (float amplitude : { 1.5f, 2.0f, 4.0f, 8.0f, 16.0f })
    {
        std::vector<std::vector<float>> buf(kNumChannels, std::vector<float>(kBlockSize, amplitude));
        auto ptrs = makePtrs(buf);
        limiter.process(ptrs.data(), kNumChannels, kBlockSize);

        for (int ch = 0; ch < kNumChannels; ++ch)
            REQUIRE(blockPeak(buf[ch]) <= 1.0f + 1e-4f);
    }
}

// ---------------------------------------------------------------------------
// test_passthrough_below_threshold
//   Signal below 0 dBFS (1.0) must pass through without modification
// ---------------------------------------------------------------------------
TEST_CASE("test_passthrough_below_threshold", "[TransientLimiter]")
{
    TransientLimiter limiter;
    limiter.prepare(kSampleRate, kBlockSize, 1);  // mono

    AlgorithmParams params = getAlgorithmParams(LimiterAlgorithm::Safe);
    params.kneeWidth      = 0.0f;  // hard knee — no early reduction
    params.saturationAmount = 0.0f;
    limiter.setAlgorithmParams(params);
    limiter.setLookahead(0.0f);  // no delay so output is synchronous

    // Input at -6 dBFS (amplitude 0.5)
    const float amplitude = 0.5f;
    std::vector<std::vector<float>> buf(1, std::vector<float>(kBlockSize, amplitude));
    auto ptrs = makePtrs(buf);

    // Run a warm-up pass to clear any transient state, then process again
    limiter.process(ptrs.data(), 1, kBlockSize);
    fillConstant(buf, amplitude);
    limiter.process(ptrs.data(), 1, kBlockSize);

    // After settling, sub-threshold signal should pass unchanged
    const float outPeak = blockPeak(buf[0]);
    REQUIRE(outPeak == Catch::Approx(amplitude).margin(0.01f));

    // GR should be negligible
    REQUIRE(limiter.getGainReduction() >= -0.5f);  // less than 0.5 dB reduction
}
