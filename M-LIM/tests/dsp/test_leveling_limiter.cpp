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
//   After exactly τ samples of silence following full engagement, the
//   remaining GR must be 1/e of the starting GR (within ±5% tolerance).
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
    const float loudSignal = 4.0f;  // +12 dBFS
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

    // After one τ, remaining GR should be grStart * (1/e) ≈ grStart * 0.3679
    // grStart is negative (e.g. -12 dB), grAfterTau should be closer to 0
    const float expected = grStart / static_cast<float>(M_E);  // grStart * 0.3679
    const float tolerance = std::abs(expected) * 0.05f;         // ±5%
    REQUIRE(grAfterTau >= expected - tolerance);
    REQUIRE(grAfterTau <= expected + tolerance);
}

// ---------------------------------------------------------------------------
// test_attack_time_constant_correct
//   With 10 ms attack at 44100 Hz the time constant τ = 441 samples.
//   After exactly τ samples of a loud signal, 63.2% of the required
//   gain reduction must be applied (within ±5% tolerance).
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

    // Use a constant +12 dBFS signal — target gain = -12 dB
    // Starting from unity (0 dB GR), after τ = 441 samples,
    // 63.2% of 12 dB = 7.58 dB of reduction should be applied.
    const float loudSignal = 4.0f;  // +12 dBFS
    const float targetGRdB = -12.0f;  // required GR in dB

    std::vector<std::vector<float>> buf(1, std::vector<float>(1, loudSignal));
    auto ptrs = makePtrs(buf);

    const int tauSamples = static_cast<int>(10.0f * 0.001f * kSampleRate);  // 441
    for (int s = 0; s < tauSamples; ++s)
    {
        buf[0][0] = loudSignal;
        limiter.process(ptrs.data(), 1, 1);
    }

    const float grAfterTau = limiter.getGainReduction();

    // After τ, 63.2% of -12 dB should be applied → GR ≈ -7.58 dB
    const float expectedGR = targetGRdB * (1.0f - 1.0f / static_cast<float>(M_E));
    const float tolerance  = std::abs(expectedGR) * 0.05f;  // ±5%
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
TEST_CASE("test_sidechain_drives_gr_not_main", "[LevelingLimiter]")
{
    LevelingLimiter limiter;
    limiter.prepare(kSampleRate, kBlockSize, 1);

    AlgorithmParams params = getAlgorithmParams(LimiterAlgorithm::Transparent);
    params.adaptiveRelease = false;
    limiter.setAlgorithmParams(params);
    limiter.setAttack(0.0f);    // instant attack
    limiter.setRelease(500.0f);
    limiter.setChannelLink(0.0f);
    limiter.setThreshold(1.0f);

    const float scAmplitude   = 2.0f;  // +6 dB over threshold
    const float mainAmplitude = 0.5f;  // well below threshold on its own

    // Warm-up blocks: loud sidechain + quiet main
    for (int block = 0; block < 30; ++block)
    {
        std::vector<std::vector<float>> main(1, std::vector<float>(kBlockSize, mainAmplitude));
        std::vector<std::vector<float>> sc  (1, std::vector<float>(kBlockSize, scAmplitude));

        float* mainPtrs[1] = { main[0].data() };
        const float* scPtrs[1] = { sc[0].data() };
        limiter.process(mainPtrs, 1, kBlockSize, scPtrs);

        if (block >= 20)
        {
            // Main should be attenuated even though it is below threshold,
            // because the loud sidechain triggered GR.
            REQUIRE(blockPeak(main[0]) < mainAmplitude);
        }
    }
}

// ---------------------------------------------------------------------------
// test_sidechain_silent_no_gr
//   When sidechainData is silent but the main audio is loud, the limiter must
//   NOT apply significant gain reduction — the envelope follower runs on the
//   silent sidechain, so the loud main audio passes substantially unattenuated.
// ---------------------------------------------------------------------------
TEST_CASE("test_sidechain_silent_no_gr", "[LevelingLimiter]")
{
    LevelingLimiter limiter;
    limiter.prepare(kSampleRate, kBlockSize, 1);

    AlgorithmParams params = getAlgorithmParams(LimiterAlgorithm::Transparent);
    params.adaptiveRelease = false;
    limiter.setAlgorithmParams(params);
    limiter.setAttack(0.0f);
    limiter.setRelease(500.0f);
    limiter.setChannelLink(0.0f);
    limiter.setThreshold(1.0f);

    const float mainAmplitude = 2.0f;  // +6 dB — would normally trigger limiting

    std::vector<std::vector<float>> main(1, std::vector<float>(kBlockSize, mainAmplitude));
    std::vector<std::vector<float>> sc  (1, std::vector<float>(kBlockSize, 0.0f));  // silence

    float* mainPtrs[1] = { main[0].data() };
    const float* scPtrs[1] = { sc[0].data() };

    // Process one block with silent sidechain + loud main
    limiter.process(mainPtrs, 1, kBlockSize, scPtrs);

    // Sidechain is silent, so GR should be minimal — main passes substantially through
    REQUIRE(blockPeak(main[0]) > 1.5f);  // expect minimal attenuation
}

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
