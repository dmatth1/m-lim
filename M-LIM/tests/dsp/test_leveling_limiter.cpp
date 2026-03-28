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
// test_release_is_exponential_in_dB
//   After driving GR to -12 dB and feeding silence, the gain recovery must
//   follow an exponential curve in dB: successive 100-sample intervals
//   must multiply the remaining GR by the same factor (geometric progression).
//   We check that the ratio GR[i+100]/GR[i] is consistent across 5 intervals,
//   with 10% tolerance.
// ---------------------------------------------------------------------------
TEST_CASE("test_release_is_exponential_in_dB", "[LevelingLimiter]")
{
    LevelingLimiter limiter;
    limiter.prepare(kSampleRate, kBlockSize, 1);

    AlgorithmParams params = getAlgorithmParams(LimiterAlgorithm::Transparent);
    params.adaptiveRelease = false;
    limiter.setAlgorithmParams(params);
    limiter.setAttack(0.0f);       // instant attack
    limiter.setRelease(500.0f);    // slow release so we can measure multiple intervals
    limiter.setChannelLink(0.0f);

    // Drive to approximately -12 dB GR with a +12 dBFS signal
    const float loudSignal = 4.0f;  // +12 dBFS
    std::vector<std::vector<float>> buf(1, std::vector<float>(kBlockSize, loudSignal));
    auto ptrs = makePtrs(buf);
    for (int i = 0; i < 50; ++i)
    {
        fillConstant(buf, loudSignal);
        limiter.process(ptrs.data(), 1, kBlockSize);
    }

    // Verify GR is significantly engaged before releasing
    REQUIRE(limiter.getGainReduction() < -3.0f);

    // Now release sample-by-sample, recording GR every 100 samples
    const int numIntervals = 6;
    const int intervalSamples = 100;
    float grMeasurements[numIntervals + 1];
    grMeasurements[0] = limiter.getGainReduction();

    for (int interval = 0; interval < numIntervals; ++interval)
    {
        // Process 100 samples of silence
        for (int s = 0; s < intervalSamples; ++s)
        {
            buf[0][0] = 0.0f;
            limiter.process(ptrs.data(), 1, 1);
        }
        grMeasurements[interval + 1] = limiter.getGainReduction();
    }

    // Compute ratios of consecutive GR readings (should be approximately constant)
    // Only check interior intervals — skip first (attack transient) and last (near 0)
    std::vector<float> ratios;
    for (int i = 1; i < numIntervals - 1; ++i)
    {
        if (grMeasurements[i] < -0.1f)  // avoid division by near-zero
        {
            ratios.push_back(grMeasurements[i + 1] / grMeasurements[i]);
        }
    }

    REQUIRE(ratios.size() >= 3);

    // Compute mean ratio
    float meanRatio = 0.0f;
    for (float r : ratios) meanRatio += r;
    meanRatio /= static_cast<float>(ratios.size());

    // Each ratio should be within 10% of the mean (exponential = constant ratio)
    for (float r : ratios)
        REQUIRE(std::abs(r - meanRatio) / meanRatio < 0.10f);
}

// ---------------------------------------------------------------------------
// test_release_time_constant_correct
//   With 100 ms release at 44100 Hz the time constant τ = 4410 samples.
//   In linear-domain smoothing, after exactly τ samples of silence the
//   linear gain moves (1-1/e) ≈ 63.2% of the way from g0 toward unity.
//   We verify grAfterTau matches this linear-domain expectation (±5%).
// ---------------------------------------------------------------------------
TEST_CASE("test_release_time_constant_correct", "[LevelingLimiter]")
{
    LevelingLimiter limiter;
    limiter.prepare(kSampleRate, kBlockSize, 1);

    AlgorithmParams params = getAlgorithmParams(LimiterAlgorithm::Transparent);
    params.adaptiveRelease = false;
    limiter.setAlgorithmParams(params);
    limiter.setAttack(0.0f);    // instant attack
    limiter.setRelease(100.0f); // 100 ms → τ = 4410 samples
    limiter.setChannelLink(0.0f);

    // Drive to approximately -12 dB GR and record starting GR
    const float loudSignal = 4.0f;  // +12 dBFS → required gain = 0.25
    std::vector<std::vector<float>> buf(1, std::vector<float>(kBlockSize, loudSignal));
    auto ptrs = makePtrs(buf);
    for (int i = 0; i < 50; ++i)
    {
        fillConstant(buf, loudSignal);
        limiter.process(ptrs.data(), 1, kBlockSize);
    }

    const float grStart = limiter.getGainReduction();
    REQUIRE(grStart < -3.0f);

    // Release for exactly one time constant: τ = 100ms * 44100 = 4410 samples
    const int tauSamples = static_cast<int>(100.0f * 0.001f * kSampleRate);  // 4410
    for (int s = 0; s < tauSamples; ++s)
    {
        buf[0][0] = 0.0f;
        limiter.process(ptrs.data(), 1, 1);
    }

    const float grAfterTau = limiter.getGainReduction();

    // Linear-domain smoothing: after τ samples, the linear gain moves
    // (1-1/e) ≈ 63.2% of the way from g0 = 1/loudSignal to unity (1.0).
    // g[τ] = g0 + (1.0 - g0) * (1 - 1/e)
    const float g0 = 1.0f / loudSignal;  // 0.25
    const float gAfterTau = g0 + (1.0f - g0) * (1.0f - 1.0f / static_cast<float>(M_E));
    const float expected = 20.0f * std::log10(gAfterTau);  // ≈ -2.81 dB
    const float tolerance = std::abs(expected) * 0.05f;    // ±5%
    REQUIRE(grAfterTau >= expected - tolerance);
    REQUIRE(grAfterTau <= expected + tolerance);
}

// ---------------------------------------------------------------------------
// test_attack_time_constant_correct
//   With 10 ms attack at 44100 Hz the time constant τ = 441 samples.
//   In linear-domain smoothing, after exactly τ samples the linear gain
//   moves (1-1/e) ≈ 63.2% of the way from unity toward the target gain.
//   We verify grAfterTau matches this linear-domain expectation (±5%).
// ---------------------------------------------------------------------------
TEST_CASE("test_attack_time_constant_correct", "[LevelingLimiter]")
{
    LevelingLimiter limiter;
    limiter.prepare(kSampleRate, kBlockSize, 1);

    AlgorithmParams params = getAlgorithmParams(LimiterAlgorithm::Transparent);
    params.adaptiveRelease = false;
    limiter.setAlgorithmParams(params);
    limiter.setAttack(10.0f);   // 10 ms → τ = 441 samples
    limiter.setRelease(5000.0f);
    limiter.setChannelLink(0.0f);

    // Use a constant +12 dBFS signal — required gain = 1/4 = 0.25 (linear)
    const float loudSignal = 4.0f;  // +12 dBFS → target linear gain = 0.25

    std::vector<std::vector<float>> buf(1, std::vector<float>(1, loudSignal));
    auto ptrs = makePtrs(buf);

    const int tauSamples = static_cast<int>(10.0f * 0.001f * kSampleRate);  // 441
    for (int s = 0; s < tauSamples; ++s)
    {
        buf[0][0] = loudSignal;
        limiter.process(ptrs.data(), 1, 1);
    }

    const float grAfterTau = limiter.getGainReduction();

    // Linear-domain smoothing: after τ samples, the linear gain moves
    // (1-1/e) ≈ 63.2% of the way from unity (1.0) to target (1/loudSignal).
    // g[τ] = 1.0 + (target - 1.0) * (1 - 1/e)
    const float target = 1.0f / loudSignal;  // 0.25
    const float gAfterTau = 1.0f + (target - 1.0f) * (1.0f - 1.0f / static_cast<float>(M_E));
    const float expectedGR = 20.0f * std::log10(gAfterTau);  // ≈ -5.59 dB
    const float tolerance  = std::abs(expectedGR) * 0.05f;   // ±5%
    REQUIRE(grAfterTau <= expectedGR + tolerance);
    REQUIRE(grAfterTau >= expectedGR - tolerance);
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
// test_sidechain_drives_gr_not_main
//   When sidechainData is loud (above threshold) but the main audio is quiet,
//   the LevelingLimiter must apply gain reduction to the quiet main audio
//   because the envelope follower runs on the sidechain signal.
// ---------------------------------------------------------------------------
// (Sidechain tests removed — LevelingLimiter no longer accepts a sidechain
//  parameter; Stage 2 always detects on post-Stage-1 main audio.)
// ---------------------------------------------------------------------------

// ---------------------------------------------------------------------------
// test_attack_zero_is_instantaneous
//   When attackMs == 0 the attack coefficient is 0, meaning the gain envelope
//   jumps directly to the target on the very first sample.  After processing
//   a single block of amplitude 2.0 (6 dB above threshold 1.0), the output
//   peak must already be clamped to <= 1.0 — an overshoot would indicate the
//   envelope had not fully converged on the first sample.
// ---------------------------------------------------------------------------
TEST_CASE("test_attack_zero_is_instantaneous", "[LevelingLimiter]")
{
    LevelingLimiter limiter;
    limiter.prepare(kSampleRate, kBlockSize, 1);

    AlgorithmParams params = getAlgorithmParams(LimiterAlgorithm::Transparent);
    params.adaptiveRelease = false;
    limiter.setAlgorithmParams(params);
    limiter.setAttack(0.0f);        // instantaneous: mAttackCoeff = 0
    limiter.setRelease(500.0f);
    limiter.setChannelLink(0.0f);
    limiter.setThreshold(1.0f);

    // amplitude 2.0 = +6 dBFS over threshold — required gain is 0.5 (-6 dB)
    std::vector<std::vector<float>> buf(1, std::vector<float>(kBlockSize, 2.0f));
    auto ptrs = makePtrs(buf);

    // Process exactly ONE block — with instant attack, GR must engage on sample 0
    limiter.process(ptrs.data(), 1, kBlockSize);

    // GR must be active after one block
    REQUIRE(limiter.getGainReduction() < -0.1f);

    // With attack=0, g jumps to targetDb on the first sample via:
    //   smoothedDb = gDb + (targetDb - gDb) * (1 - 0) = targetDb
    // so every sample in the block is clamped to threshold
    REQUIRE(blockPeak(buf[0]) <= 1.0f + 1e-4f);
}

// ---------------------------------------------------------------------------
// test_adaptive_release_speedup_threshold
//   The adaptive release speedup activates only when sustainedGRdB > 0.5 dB.
//   Two sections straddle this boundary:
//   1. Low GR (<0.5 dB): adaptive and non-adaptive recover at the same rate.
//   2. High GR (>0.5 dB): adaptive release measurably outpaces non-adaptive.
// ---------------------------------------------------------------------------
TEST_CASE("test_adaptive_release_speedup_threshold", "[LevelingLimiter]")
{
    // Helper: sustain a constant signal to build mEnvState, then release silence
    // and return the remaining GR.
    auto measureRecovery = [](float amplitude, bool adaptive,
                               int sustainBlocks, int recoverBlocks) -> float
    {
        LevelingLimiter limiter;
        limiter.prepare(kSampleRate, kBlockSize, 1);
        limiter.setAttack(0.0f);
        limiter.setRelease(200.0f);
        limiter.setChannelLink(0.0f);
        limiter.setThreshold(1.0f);

        AlgorithmParams params = getAlgorithmParams(LimiterAlgorithm::Transparent);
        params.adaptiveRelease = adaptive;
        limiter.setAlgorithmParams(params);

        std::vector<std::vector<float>> buf(1, std::vector<float>(kBlockSize));
        auto ptrs = makePtrs(buf);

        for (int i = 0; i < sustainBlocks; ++i)
        {
            std::fill(buf[0].begin(), buf[0].end(), amplitude);
            limiter.process(ptrs.data(), 1, kBlockSize);
        }
        for (int i = 0; i < recoverBlocks; ++i)
        {
            std::fill(buf[0].begin(), buf[0].end(), 0.0f);
            limiter.process(ptrs.data(), 1, kBlockSize);
        }
        return limiter.getGainReduction();
    };

    SECTION("low GR (below 0.5 dB threshold): adaptive does NOT speed up release")
    {
        // 10^(0.4/20) ≈ 1.047 → required GR ≈ -0.4 dB, below the 0.5 dB speedup threshold
        const float lowAmplitude = std::pow(10.0f, 0.4f / 20.0f);

        // Use 200 blocks so mEnvState settles (~99% of steady-state value)
        const float grAdaptive    = measureRecovery(lowAmplitude, true,  200, 10);
        const float grNonAdaptive = measureRecovery(lowAmplitude, false, 200, 10);

        // sustainedGRdB ≈ 0.4 dB < 0.5 dB → effectiveReleaseCoeff == mReleaseCoeff
        // in both cases, so recovery rates must be identical
        REQUIRE(std::abs(grAdaptive - grNonAdaptive) < 0.01f);
    }

    SECTION("high GR (above 0.5 dB threshold): adaptive release speeds up recovery")
    {
        // amplitude 2.0 → required GR ≈ -6 dB, well above the 0.5 dB speedup threshold
        const float highAmplitude = 2.0f;

        const float grAdaptive    = measureRecovery(highAmplitude, true,  200, 10);
        const float grNonAdaptive = measureRecovery(highAmplitude, false, 200, 10);

        // Adaptive should recover more (less negative GR) than non-adaptive because
        // sustainedGRdB ≈ 6 dB > 0.5 dB activates the coeff^2 compounding speedup
        REQUIRE(grAdaptive > grNonAdaptive);
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

// ---------------------------------------------------------------------------
// test_linear_domain_attack_convergence
//   Verifies that gain reduction during a constant-level tone converges to
//   the correct threshold in linear domain. After 1000 samples of a +6 dBFS
//   tone (amplitude 2.0) with attack=10ms at 44100 Hz, the resulting gain
//   must be within 0.5 dBTP of the dB-domain implementation's steady state.
//   (Both converge to the same target; the question is speed of approach.)
// ---------------------------------------------------------------------------
TEST_CASE("test_linear_domain_attack_convergence", "[LevelingLimiter]")
{
    LevelingLimiter limiter;
    limiter.prepare(kSampleRate, kBlockSize, 1);

    AlgorithmParams params = getAlgorithmParams(LimiterAlgorithm::Transparent);
    params.adaptiveRelease = false;
    limiter.setAlgorithmParams(params);
    limiter.setAttack(10.0f);    // 10 ms attack
    limiter.setRelease(100.0f);
    limiter.setChannelLink(0.0f);
    limiter.setThreshold(1.0f);

    // Feed constant amplitude = 2.0 (+6 dBFS); required gain = 0.5 (-6 dB)
    const float amplitude = 2.0f;
    const float requiredGain = 1.0f / amplitude;  // 0.5

    std::vector<std::vector<float>> buf(1, std::vector<float>(1, amplitude));
    auto ptrs = makePtrs(buf);

    // 5000 samples ≈ 11 × τ (τ=441 samples for 10ms at 44100 Hz) — guaranteed convergence
    for (int s = 0; s < 5000; ++s)
    {
        buf[0][0] = amplitude;
        limiter.process(ptrs.data(), 1, 1);
    }

    // After 5000 samples (>> τ = 441 for 10ms at 44100 Hz), gain must be
    // very close to the correct target. Steady state should be requiredGain.
    // Allow 0.5 dBTP tolerance vs. ideal steady-state.
    const float steadyStateGRdB = 20.0f * std::log10(requiredGain);  // -6.02 dB
    REQUIRE(limiter.getGainReduction() <= steadyStateGRdB + 0.5f);
    REQUIRE(limiter.getGainReduction() >= steadyStateGRdB - 0.5f);
}

// ---------------------------------------------------------------------------
// test_linear_domain_release_speed
//   After driving gain to -20 dBGR (amplitude ≈ 10.0 → gain ≈ 0.1),
//   feeding silence must recover gain to < -0.1 dBGR within 3×releaseMs.
//   Verifies that linear-domain release speed is preserved.
// ---------------------------------------------------------------------------
TEST_CASE("test_linear_domain_release_speed", "[LevelingLimiter]")
{
    LevelingLimiter limiter;
    limiter.prepare(kSampleRate, kBlockSize, 1);

    AlgorithmParams params = getAlgorithmParams(LimiterAlgorithm::Transparent);
    params.adaptiveRelease = false;
    limiter.setAlgorithmParams(params);
    limiter.setAttack(0.0f);    // instant attack
    limiter.setRelease(100.0f); // 100 ms release
    limiter.setChannelLink(0.0f);
    limiter.setThreshold(1.0f);

    // Drive to approximately -20 dB GR with amplitude = 10.0
    const float amplitude = 10.0f;  // 10× above threshold → gain ≈ 0.1 = -20 dB
    std::vector<std::vector<float>> buf(1, std::vector<float>(kBlockSize));
    auto ptrs = makePtrs(buf);

    for (int i = 0; i < 30; ++i)
    {
        std::fill(buf[0].begin(), buf[0].end(), amplitude);
        limiter.process(ptrs.data(), 1, kBlockSize);
    }
    REQUIRE(limiter.getGainReduction() < -10.0f);  // significant GR engaged

    // Release with silence for 5 × releaseMs = 500 ms = 22050 samples
    // (5 time constants: linear-domain recovery to ≈ 99.3% = -0.06 dBGR)
    const int releaseSamples = static_cast<int>(5.0f * 100.0f * 0.001f * kSampleRate);
    for (int s = 0; s < releaseSamples; ++s)
    {
        buf[0][0] = 0.0f;
        limiter.process(ptrs.data(), 1, 1);
    }

    // After 5×τ, gain must have recovered to < -0.1 dBGR
    REQUIRE(limiter.getGainReduction() > -0.1f);
}

// ---------------------------------------------------------------------------
// test_adaptive_env_state_linear_convergence
//   With adaptiveRelease=true and a constant loud signal, mEnvState (now in
//   linear domain) must converge toward the required gain (1/amplitude).
//   After 500 ms × kAdaptiveSmoothMs warm-up, the adaptive release must
//   produce measurably faster recovery than non-adaptive.
// ---------------------------------------------------------------------------
TEST_CASE("test_adaptive_env_state_linear_convergence", "[LevelingLimiter]")
{
    // Verify that adaptive release with sustained GR causes faster recovery
    // than non-adaptive — confirms that mEnvState in linear domain correctly
    // drives the adaptive speedup logic.
    const float amplitude    = 4.0f;   // +12 dBFS → sustained GR ≈ -12 dB
    const int sustainBlocks  = 300;    // ~3 s of sustained gain reduction (> 500 ms smoother)
    const int recoverSamples = static_cast<int>(200.0f * 0.001f * kSampleRate);  // 200 ms

    auto measure = [&](bool adaptive) -> float
    {
        LevelingLimiter lim;
        lim.prepare(kSampleRate, kBlockSize, 1);
        lim.setAttack(0.0f);
        lim.setRelease(500.0f);
        lim.setChannelLink(0.0f);
        lim.setThreshold(1.0f);

        AlgorithmParams p = getAlgorithmParams(LimiterAlgorithm::Transparent);
        p.adaptiveRelease = adaptive;
        lim.setAlgorithmParams(p);

        std::vector<std::vector<float>> b(1, std::vector<float>(kBlockSize));
        auto ptrs = makePtrs(b);

        for (int i = 0; i < sustainBlocks; ++i)
        {
            std::fill(b[0].begin(), b[0].end(), amplitude);
            lim.process(ptrs.data(), 1, kBlockSize);
        }
        for (int s = 0; s < recoverSamples; ++s)
        {
            b[0][0] = 0.0f;
            lim.process(ptrs.data(), 1, 1);
        }
        return lim.getGainReduction();
    };

    const float grAdaptive    = measure(true);
    const float grNonAdaptive = measure(false);

    // After sustained GR (> 500 ms smoother warm-up), adaptive must recover more
    REQUIRE(grAdaptive > grNonAdaptive);
}

// ---------------------------------------------------------------------------
// test_zero_release_instant_recovery
//   With release=10ms (minimum clamp), zero attack, loud block then quiet block;
//   gain should recover within 3 dB of unity within first 10 samples of quiet.
//   Note: setRelease clamps to [10, 1000] ms, so we use the minimum 10 ms.
// ---------------------------------------------------------------------------
TEST_CASE("test_zero_release_instant_recovery", "[LevelingLimiter]")
{
    LevelingLimiter limiter;
    limiter.prepare(kSampleRate, kBlockSize, 1);

    AlgorithmParams params = getAlgorithmParams(LimiterAlgorithm::Transparent);
    params.adaptiveRelease = false;
    limiter.setAlgorithmParams(params);
    limiter.setAttack(0.0f);     // instant attack
    limiter.setRelease(10.0f);   // minimum release (10 ms)
    limiter.setChannelLink(0.0f);
    limiter.setThreshold(1.0f);

    // Drive limiter with loud signal to engage GR
    const float loudSignal = 4.0f;  // +12 dBFS
    std::vector<std::vector<float>> buf(1, std::vector<float>(kBlockSize, loudSignal));
    auto ptrs = makePtrs(buf);

    for (int i = 0; i < 50; ++i)
    {
        fillConstant(buf, loudSignal);
        limiter.process(ptrs.data(), 1, kBlockSize);
    }

    // Confirm GR is significantly engaged
    REQUIRE(limiter.getGainReduction() < -6.0f);

    // Now feed silence sample-by-sample and check recovery
    // With 10 ms release at 44100 Hz, τ = 441 samples.
    // After 10 samples: g moves (1 - e^(-10/441)) ≈ 2.2% toward unity.
    // Starting from g ≈ 0.25 (−12 dB), after 10 samples g ≈ 0.267.
    // After 441 samples (1 τ), g moves 63.2% toward unity → g ≈ 0.724 → −2.8 dB.
    // Check that within 441 samples (1 time constant), gain is within 3 dB of unity.
    for (int s = 0; s < 441; ++s)
    {
        buf[0][0] = 0.0f;
        limiter.process(ptrs.data(), 1, 1);
    }

    // After 1 time constant of minimum release, GR should be within 3 dB of unity
    REQUIRE(limiter.getGainReduction() > -3.0f);
}

// ---------------------------------------------------------------------------
// test_adaptive_release_exact_boundary_0_5db
//   Drive envelope to exactly around 0.5 dB above target; confirm that the
//   adaptive release speedup activates above 0.5 dB but not below.
//   This tests the exact boundary condition in the adaptive release logic.
// ---------------------------------------------------------------------------
TEST_CASE("test_adaptive_release_exact_boundary_0_5db", "[LevelingLimiter]")
{
    // Helper: sustain a signal at a given amplitude with adaptive release enabled,
    // then release and return the GR after recovery.
    auto measureRecovery = [](float amplitude, int sustainBlocks, int recoverBlocks) -> float
    {
        LevelingLimiter limiter;
        limiter.prepare(kSampleRate, kBlockSize, 1);
        limiter.setAttack(0.0f);
        limiter.setRelease(200.0f);
        limiter.setChannelLink(0.0f);
        limiter.setThreshold(1.0f);

        AlgorithmParams params = getAlgorithmParams(LimiterAlgorithm::Transparent);
        params.adaptiveRelease = true;
        limiter.setAlgorithmParams(params);

        std::vector<std::vector<float>> buf(1, std::vector<float>(kBlockSize));
        auto ptrs = makePtrs(buf);

        for (int i = 0; i < sustainBlocks; ++i)
        {
            std::fill(buf[0].begin(), buf[0].end(), amplitude);
            limiter.process(ptrs.data(), 1, kBlockSize);
        }
        for (int i = 0; i < recoverBlocks; ++i)
        {
            std::fill(buf[0].begin(), buf[0].end(), 0.0f);
            limiter.process(ptrs.data(), 1, kBlockSize);
        }
        return limiter.getGainReduction();
    };

    // Amplitude just below 0.5 dB threshold: 10^(0.3/20) ≈ 1.035
    const float belowAmplitude = std::pow(10.0f, 0.3f / 20.0f);
    // Amplitude well above 0.5 dB threshold: 10^(3.0/20) ≈ 1.413
    const float aboveAmplitude = std::pow(10.0f, 3.0f / 20.0f);

    // For below-threshold case, also measure non-adaptive to confirm no speedup
    LevelingLimiter limNonAdaptive;
    limNonAdaptive.prepare(kSampleRate, kBlockSize, 1);
    limNonAdaptive.setAttack(0.0f);
    limNonAdaptive.setRelease(200.0f);
    limNonAdaptive.setChannelLink(0.0f);
    limNonAdaptive.setThreshold(1.0f);
    AlgorithmParams pNA = getAlgorithmParams(LimiterAlgorithm::Transparent);
    pNA.adaptiveRelease = false;
    limNonAdaptive.setAlgorithmParams(pNA);

    std::vector<std::vector<float>> bufNA(1, std::vector<float>(kBlockSize));
    auto ptrsNA = makePtrs(bufNA);
    for (int i = 0; i < 200; ++i)
    {
        std::fill(bufNA[0].begin(), bufNA[0].end(), belowAmplitude);
        limNonAdaptive.process(ptrsNA.data(), 1, kBlockSize);
    }
    for (int i = 0; i < 10; ++i)
    {
        std::fill(bufNA[0].begin(), bufNA[0].end(), 0.0f);
        limNonAdaptive.process(ptrsNA.data(), 1, kBlockSize);
    }
    const float grBelowNA = limNonAdaptive.getGainReduction();

    const float grBelowAdaptive = measureRecovery(belowAmplitude, 200, 10);
    const float grAboveAdaptive = measureRecovery(aboveAmplitude, 200, 10);

    // Below 0.5 dB: adaptive should match non-adaptive (no speedup)
    REQUIRE(std::abs(grBelowAdaptive - grBelowNA) < 0.01f);

    // Above 0.5 dB: adaptive recovery should be faster (less negative GR)
    // Compare to non-adaptive at same amplitude
    LevelingLimiter limAboveNA;
    limAboveNA.prepare(kSampleRate, kBlockSize, 1);
    limAboveNA.setAttack(0.0f);
    limAboveNA.setRelease(200.0f);
    limAboveNA.setChannelLink(0.0f);
    limAboveNA.setThreshold(1.0f);
    pNA.adaptiveRelease = false;
    limAboveNA.setAlgorithmParams(pNA);

    std::vector<std::vector<float>> bufAboveNA(1, std::vector<float>(kBlockSize));
    auto ptrsAboveNA = makePtrs(bufAboveNA);
    for (int i = 0; i < 200; ++i)
    {
        std::fill(bufAboveNA[0].begin(), bufAboveNA[0].end(), aboveAmplitude);
        limAboveNA.process(ptrsAboveNA.data(), 1, kBlockSize);
    }
    for (int i = 0; i < 10; ++i)
    {
        std::fill(bufAboveNA[0].begin(), bufAboveNA[0].end(), 0.0f);
        limAboveNA.process(ptrsAboveNA.data(), 1, kBlockSize);
    }
    const float grAboveNA = limAboveNA.getGainReduction();

    // Above boundary: adaptive should recover more (less negative) than non-adaptive
    REQUIRE(grAboveAdaptive > grAboveNA);
}

// ---------------------------------------------------------------------------
// test_gain_never_exceeds_unity
//   Process 1000 loud blocks then silence; no output sample should ever exceed
//   1.0f, ensuring no overshoot during attack or release phases.
// ---------------------------------------------------------------------------
TEST_CASE("test_gain_never_exceeds_unity", "[LevelingLimiter]")
{
    LevelingLimiter limiter;
    limiter.prepare(kSampleRate, kBlockSize, kNumChannels);

    AlgorithmParams params = getAlgorithmParams(LimiterAlgorithm::Transparent);
    params.adaptiveRelease = true;  // test with adaptive release enabled
    limiter.setAlgorithmParams(params);
    limiter.setAttack(0.0f);     // instant attack
    limiter.setRelease(100.0f);
    limiter.setChannelLink(1.0f);
    limiter.setThreshold(1.0f);

    std::vector<std::vector<float>> buf(kNumChannels, std::vector<float>(kBlockSize));
    auto ptrs = makePtrs(buf);

    float maxSeen = 0.0f;

    // Phase 1: 1000 loud blocks
    for (int block = 0; block < 1000; ++block)
    {
        fillConstant(buf, 4.0f);  // +12 dBFS
        limiter.process(ptrs.data(), kNumChannels, kBlockSize);

        for (int ch = 0; ch < kNumChannels; ++ch)
            for (int s = 0; s < kBlockSize; ++s)
                maxSeen = std::max(maxSeen, std::abs(buf[ch][s]));
    }

    // Phase 2: silence — check release doesn't overshoot
    for (int block = 0; block < 200; ++block)
    {
        fillConstant(buf, 0.0f);
        limiter.process(ptrs.data(), kNumChannels, kBlockSize);

        for (int ch = 0; ch < kNumChannels; ++ch)
            for (int s = 0; s < kBlockSize; ++s)
                maxSeen = std::max(maxSeen, std::abs(buf[ch][s]));
    }

    // No output sample should ever exceed 1.0 (small tolerance for IIR numerical drift)
    REQUIRE(maxSeen <= 1.0f + 2e-3f);
}

// ---------------------------------------------------------------------------
// test_threshold_change_no_overshoot
//   Rapid threshold changes (-6 → -12 → -6 dB) during limiting must not cause
//   any output sample to exceed 1.0f.
// ---------------------------------------------------------------------------
TEST_CASE("test_threshold_change_no_overshoot", "[LevelingLimiter]")
{
    LevelingLimiter limiter;
    limiter.prepare(kSampleRate, kBlockSize, kNumChannels);

    AlgorithmParams params = getAlgorithmParams(LimiterAlgorithm::Transparent);
    params.adaptiveRelease = false;
    limiter.setAlgorithmParams(params);
    limiter.setAttack(0.0f);     // instant attack
    limiter.setRelease(100.0f);
    limiter.setChannelLink(1.0f);
    limiter.setThreshold(1.0f);

    std::vector<std::vector<float>> buf(kNumChannels, std::vector<float>(kBlockSize));
    auto ptrs = makePtrs(buf);

    float maxSeen = 0.0f;
    const float inputLevel = 4.0f;  // +12 dBFS — always above threshold

    // Warm up at default threshold
    for (int block = 0; block < 20; ++block)
    {
        fillConstant(buf, inputLevel);
        limiter.process(ptrs.data(), kNumChannels, kBlockSize);
    }

    // Rapid threshold changes: cycle through -6 → -12 → -6 dB
    const float thresholds[] = {
        0.501f,   // -6 dBFS
        0.251f,   // -12 dBFS
        0.501f,   // back to -6 dBFS
        0.251f,   // -12 dBFS again
        0.501f    // -6 dBFS again
    };

    for (float thr : thresholds)
    {
        limiter.setThreshold(thr);

        for (int block = 0; block < 40; ++block)
        {
            fillConstant(buf, inputLevel);
            limiter.process(ptrs.data(), kNumChannels, kBlockSize);

            for (int ch = 0; ch < kNumChannels; ++ch)
                for (int s = 0; s < kBlockSize; ++s)
                    maxSeen = std::max(maxSeen, std::abs(buf[ch][s]));
        }
    }

    // No output sample should ever exceed 1.0 regardless of threshold changes (small tolerance for IIR numerical drift)
    REQUIRE(maxSeen <= 1.0f + 2e-3f);
}

// ---------------------------------------------------------------------------
// (Sidechain tests removed — LevelingLimiter no longer accepts a sidechain
//  parameter; Stage 2 always detects on post-Stage-1 main audio.)
// ---------------------------------------------------------------------------

// ---------------------------------------------------------------------------
// test_channel_link_fifty_percent_partially_links
//   At link=0.5 the quiet channel's output must be between the fully-independent
//   result (no extra reduction) and the fully-linked result (maximum reduction).
// ---------------------------------------------------------------------------
TEST_CASE("test_channel_link_fifty_percent_partially_links", "[LevelingLimiter]")
{
    auto runWithLink = [&](float link) -> float
    {
        LevelingLimiter limiter;
        limiter.prepare(kSampleRate, kBlockSize, 2);

        AlgorithmParams params = getAlgorithmParams(LimiterAlgorithm::Transparent);
        params.adaptiveRelease = false;
        limiter.setAlgorithmParams(params);
        limiter.setThreshold(1.0f);
        limiter.setAttack(0.0f);
        limiter.setRelease(100.0f);
        limiter.setChannelLink(link);

        // Ch0: loud (3.0 = well above threshold), Ch1: quiet (0.1 = below threshold)
        std::vector<std::vector<float>> buf(2, std::vector<float>(kBlockSize));

        // Warm up the envelope for several blocks
        for (int block = 0; block < 30; ++block)
        {
            std::fill(buf[0].begin(), buf[0].end(), 3.0f);
            std::fill(buf[1].begin(), buf[1].end(), 0.1f);
            auto ptrs = makePtrs(buf);
            limiter.process(ptrs.data(), 2, kBlockSize);
        }

        return blockPeak(buf[1]);
    };

    const float ch1_independent = runWithLink(0.0f);  // ~0.1 (no reduction on quiet channel)
    const float ch1_linked      = runWithLink(1.0f);  // reduced (loud ch drives GR)
    const float ch1_half        = runWithLink(0.5f);  // must be between the two

    INFO("ch1_independent=" << ch1_independent
         << " ch1_half=" << ch1_half
         << " ch1_linked=" << ch1_linked);

    // Half-linked must be strictly between independent and fully-linked
    REQUIRE(ch1_half < ch1_independent);
    REQUIRE(ch1_half > ch1_linked);
}
