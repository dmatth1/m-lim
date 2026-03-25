#include "catch2/catch_amalgamated.hpp"
#include "dsp/LevelingLimiter.h"
#include "dsp/LimiterAlgorithm.h"
#include <cmath>
#include <vector>
#include <algorithm>

static constexpr double kSampleRate  = 44100.0;
static constexpr int    kBlockSize   = 512;
static constexpr int    kNumChannels = 2;

// ---------------------------------------------------------------------------
// Helpers
// ---------------------------------------------------------------------------
static std::vector<float*> makePtrs(std::vector<std::vector<float>>& buf)
{
    std::vector<float*> ptrs(buf.size());
    for (size_t i = 0; i < buf.size(); ++i)
        ptrs[i] = buf[i].data();
    return ptrs;
}

static void fillConstant(std::vector<std::vector<float>>& buf, float v)
{
    for (auto& ch : buf)
        std::fill(ch.begin(), ch.end(), v);
}

static float blockPeak(const std::vector<float>& data)
{
    float peak = 0.0f;
    for (float v : data)
        peak = std::max(peak, std::abs(v));
    return peak;
}

// ---------------------------------------------------------------------------
// test_release_envelope
//   After gain reduction ceases (signal drops to silence), the gain should
//   recover toward unity within approximately the configured release time.
// ---------------------------------------------------------------------------
TEST_CASE("test_release_envelope", "[LevelingLimiter]")
{
    LevelingLimiter limiter;
    limiter.prepare(kSampleRate, kBlockSize, 1);  // mono

    AlgorithmParams params = getAlgorithmParams(LimiterAlgorithm::Transparent);
    params.adaptiveRelease = false;
    limiter.setAlgorithmParams(params);
    limiter.setAttack(0.0f);    // instant attack so GR engages immediately
    limiter.setChannelLink(0.0f);

    const float releaseMs = 200.0f;
    limiter.setRelease(releaseMs);

    // Feed a loud signal (+6 dB) for many blocks to drive gain down
    std::vector<std::vector<float>> buf(1, std::vector<float>(kBlockSize, 2.0f));
    auto ptrs = makePtrs(buf);
    for (int i = 0; i < 20; ++i)
    {
        fillConstant(buf, 2.0f);
        limiter.process(ptrs.data(), 1, kBlockSize);
    }

    // GR should be active now
    REQUIRE(limiter.getGainReduction() < -0.1f);

    // Now feed silence — gain should start recovering
    const int releaseSamples = static_cast<int>(releaseMs * 0.001 * kSampleRate);
    // Process 3x the release time worth of silence
    const int silenceBlocks = (releaseSamples * 3) / kBlockSize + 1;
    for (int i = 0; i < silenceBlocks; ++i)
    {
        fillConstant(buf, 0.0f);
        limiter.process(ptrs.data(), 1, kBlockSize);
    }

    // After silence, GR should have recovered close to 0 dB
    REQUIRE(limiter.getGainReduction() >= -1.0f);  // less than 1 dB reduction remaining
}

// ---------------------------------------------------------------------------
// test_attack_delays_reduction
//   With a slow attack setting, gain reduction should NOT engage instantly
//   when a loud signal appears — there should be an initial period where
//   output exceeds the threshold before GR catches up.
// ---------------------------------------------------------------------------
TEST_CASE("test_attack_delays_reduction", "[LevelingLimiter]")
{
    LevelingLimiter limiter;
    limiter.prepare(kSampleRate, kBlockSize, 1);

    AlgorithmParams params = getAlgorithmParams(LimiterAlgorithm::Transparent);
    params.adaptiveRelease = false;
    limiter.setAlgorithmParams(params);
    limiter.setChannelLink(0.0f);

    const float attackMs = 50.0f;
    limiter.setAttack(attackMs);
    limiter.setRelease(500.0f);

    // A single short block with a loud input right at the start
    // With 50 ms attack at 44100 Hz, the gain should barely move in the first block
    const int attackSamples = static_cast<int>(attackMs * 0.001 * kSampleRate);

    std::vector<std::vector<float>> buf(1, std::vector<float>(kBlockSize, 2.0f));
    auto ptrs = makePtrs(buf);

    // Process one small block (much shorter than attack time)
    const int shortBlock = std::min(kBlockSize, attackSamples / 10);
    limiter.process(ptrs.data(), 1, shortBlock);

    // After a very short window with slow attack, GR should be minimal
    // (gain has barely moved from unity)
    REQUIRE(limiter.getGainReduction() > -6.0f);  // less than 6 dB of GR after tiny block

    // Now process many blocks to let the slow attack engage
    for (int i = 0; i < 20; ++i)
    {
        fillConstant(buf, 2.0f);
        limiter.process(ptrs.data(), 1, kBlockSize);
    }

    // After many blocks, GR should be active and significant
    REQUIRE(limiter.getGainReduction() < -0.5f);
}

// ---------------------------------------------------------------------------
// test_channel_linking
//   With 100% channel link, both channels must receive identical gain
//   reduction even when only one channel is loud.
// ---------------------------------------------------------------------------
TEST_CASE("test_channel_linking", "[LevelingLimiter]")
{
    SECTION("fully linked: both channels get same GR")
    {
        LevelingLimiter limiter;
        limiter.prepare(kSampleRate, kBlockSize, 2);
        limiter.setAttack(0.0f);   // instant attack
        limiter.setRelease(500.0f);
        limiter.setChannelLink(1.0f);

        AlgorithmParams params = getAlgorithmParams(LimiterAlgorithm::Transparent);
        params.adaptiveRelease = false;
        limiter.setAlgorithmParams(params);

        // Ch0: +6 dB (loud), Ch1: -6 dB (quiet — below threshold)
        std::vector<std::vector<float>> buf(2, std::vector<float>(kBlockSize));
        std::fill(buf[0].begin(), buf[0].end(), 2.0f);   // above threshold
        std::fill(buf[1].begin(), buf[1].end(), 0.5f);   // well below threshold

        auto ptrs = makePtrs(buf);

        // Warm up to engage GR
        for (int i = 0; i < 10; ++i)
        {
            std::fill(buf[0].begin(), buf[0].end(), 2.0f);
            std::fill(buf[1].begin(), buf[1].end(), 0.5f);
            limiter.process(ptrs.data(), 2, kBlockSize);
        }

        // With full linking, ch1 should be attenuated by GR from ch0
        // If independent: ch1 output ≈ 0.5; with linking: ch1 < 0.5
        const float ch1Peak = blockPeak(buf[1]);
        REQUIRE(ch1Peak < 0.45f);   // linked GR pulls quieter channel down
    }

    SECTION("independent: quiet channel passes unmodified")
    {
        LevelingLimiter limiter;
        limiter.prepare(kSampleRate, kBlockSize, 2);
        limiter.setAttack(0.0f);
        limiter.setRelease(500.0f);
        limiter.setChannelLink(0.0f);  // fully independent

        AlgorithmParams params = getAlgorithmParams(LimiterAlgorithm::Transparent);
        params.adaptiveRelease = false;
        limiter.setAlgorithmParams(params);

        // Ch0: loud, Ch1: quiet (below threshold)
        std::vector<std::vector<float>> buf(2, std::vector<float>(kBlockSize));

        auto ptrs = makePtrs(buf);
        for (int i = 0; i < 10; ++i)
        {
            std::fill(buf[0].begin(), buf[0].end(), 2.0f);
            std::fill(buf[1].begin(), buf[1].end(), 0.5f);
            limiter.process(ptrs.data(), 2, kBlockSize);
        }

        // Ch1 is below threshold and independent — should pass without attenuation
        const float ch1Peak = blockPeak(buf[1]);
        REQUIRE(ch1Peak >= 0.45f);  // no significant reduction on quiet independent channel
    }
}

// ---------------------------------------------------------------------------
// test_adaptive_release
//   When adaptiveRelease is enabled, a sustained loud signal should cause the
//   release to speed up compared to a limiter with adaptiveRelease disabled.
//   We verify this by measuring how quickly GR recovers after a sustained burst.
// ---------------------------------------------------------------------------
TEST_CASE("test_adaptive_release", "[LevelingLimiter]")
{
    const float releaseMs = 500.0f;
    const int sustainBlocks = 200;   // ~2 s of sustained gain reduction
    const int recoverBlocks = 10;    // blocks of silence to measure recovery

    // Helper lambda: measure GR after sustained loud then silence
    auto measureRecovery = [&](bool adaptive) -> float
    {
        LevelingLimiter limiter;
        limiter.prepare(kSampleRate, kBlockSize, 1);
        limiter.setAttack(0.0f);
        limiter.setRelease(releaseMs);
        limiter.setChannelLink(0.0f);

        AlgorithmParams params = getAlgorithmParams(LimiterAlgorithm::Transparent);
        params.adaptiveRelease = adaptive;
        limiter.setAlgorithmParams(params);

        std::vector<std::vector<float>> buf(1, std::vector<float>(kBlockSize));
        auto ptrs = makePtrs(buf);

        // Sustained loud signal to build up GR
        for (int i = 0; i < sustainBlocks; ++i)
        {
            std::fill(buf[0].begin(), buf[0].end(), 2.0f);
            limiter.process(ptrs.data(), 1, kBlockSize);
        }

        // Silence — measure recovery
        for (int i = 0; i < recoverBlocks; ++i)
        {
            std::fill(buf[0].begin(), buf[0].end(), 0.0f);
            limiter.process(ptrs.data(), 1, kBlockSize);
        }

        return limiter.getGainReduction();  // closer to 0 = more recovered
    };

    const float grWithAdaptive    = measureRecovery(true);
    const float grWithoutAdaptive = measureRecovery(false);

    // Adaptive release should recover more (closer to 0 dB) than non-adaptive
    // i.e., grWithAdaptive > grWithoutAdaptive (less negative = more recovered)
    REQUIRE(grWithAdaptive > grWithoutAdaptive);
}

// ---------------------------------------------------------------------------
// test_custom_threshold_minus_1dBFS
//   setThreshold(0.891) must hold output ≤ 0.891 + tiny margin.
//   LevelingLimiter uses a slow envelope, so 30+ warm-up blocks are needed.
// ---------------------------------------------------------------------------
TEST_CASE("test_custom_threshold_minus_1dBFS", "[LevelingLimiter]")
{
    LevelingLimiter limiter;
    limiter.prepare(kSampleRate, kBlockSize, 1);

    AlgorithmParams params = getAlgorithmParams(LimiterAlgorithm::Transparent);
    params.adaptiveRelease = false;
    limiter.setAlgorithmParams(params);
    limiter.setAttack(0.0f);   // instant attack
    limiter.setRelease(100.0f);
    limiter.setChannelLink(0.0f);
    limiter.setThreshold(0.891f);  // -1 dBFS

    std::vector<std::vector<float>> buf(1, std::vector<float>(kBlockSize, 2.0f));
    auto ptrs = makePtrs(buf);

    for (int block = 0; block < 40; ++block)
    {
        fillConstant(buf, 2.0f);
        limiter.process(ptrs.data(), 1, kBlockSize);
        if (block > 30)  // allow slow envelope to fully engage
            REQUIRE(blockPeak(buf[0]) <= 0.891f + 2e-3f);
    }
}

// ---------------------------------------------------------------------------
// test_custom_threshold_minus_6dBFS
//   setThreshold(0.501) at -6 dBFS must clamp a +6 dBFS input to ≤ 0.501.
// ---------------------------------------------------------------------------
TEST_CASE("test_custom_threshold_minus_6dBFS", "[LevelingLimiter]")
{
    LevelingLimiter limiter;
    limiter.prepare(kSampleRate, kBlockSize, 1);

    AlgorithmParams params = getAlgorithmParams(LimiterAlgorithm::Transparent);
    params.adaptiveRelease = false;
    limiter.setAlgorithmParams(params);
    limiter.setAttack(0.0f);
    limiter.setRelease(100.0f);
    limiter.setChannelLink(0.0f);
    limiter.setThreshold(0.501f);  // -6 dBFS

    std::vector<std::vector<float>> buf(1, std::vector<float>(kBlockSize, 2.0f));
    auto ptrs = makePtrs(buf);

    for (int block = 0; block < 40; ++block)
    {
        fillConstant(buf, 2.0f);
        limiter.process(ptrs.data(), 1, kBlockSize);
        if (block > 30)
            REQUIRE(blockPeak(buf[0]) <= 0.501f + 2e-3f);
    }
}

// ---------------------------------------------------------------------------
// test_threshold_change_mid_session
//   After changing threshold from 1.0 to 0.5 mid-session, subsequent output
//   must not exceed 0.5 + margin once the slow envelope settles.
// ---------------------------------------------------------------------------
TEST_CASE("test_threshold_change_mid_session", "[LevelingLimiter]")
{
    LevelingLimiter limiter;
    limiter.prepare(kSampleRate, kBlockSize, 1);

    AlgorithmParams params = getAlgorithmParams(LimiterAlgorithm::Transparent);
    params.adaptiveRelease = false;
    limiter.setAlgorithmParams(params);
    limiter.setAttack(0.0f);
    limiter.setRelease(100.0f);
    limiter.setChannelLink(0.0f);
    limiter.setThreshold(1.0f);  // start at default

    std::vector<std::vector<float>> buf(1, std::vector<float>(kBlockSize, 2.0f));
    auto ptrs = makePtrs(buf);

    // Phase 1: process with threshold=1.0
    for (int block = 0; block < 20; ++block)
    {
        fillConstant(buf, 2.0f);
        limiter.process(ptrs.data(), 1, kBlockSize);
    }

    // Change threshold mid-session
    limiter.setThreshold(0.5f);

    // Phase 2: process with threshold=0.5 — new threshold must be enforced
    for (int block = 0; block < 40; ++block)
    {
        fillConstant(buf, 2.0f);
        limiter.process(ptrs.data(), 1, kBlockSize);
        if (block > 30)  // allow slow envelope to settle to new threshold
            REQUIRE(blockPeak(buf[0]) <= 0.5f + 2e-3f);
    }
}
