#include "catch2/catch_amalgamated.hpp"
#include "dsp/LimiterAlgorithm.h"
#include "dsp/TransientLimiter.h"
#include "dsp/LevelingLimiter.h"
#include <cmath>
#include <vector>
#include <algorithm>
#include <string>

static constexpr int NUM_ALGORITHMS = 8;

static LimiterAlgorithm allAlgorithms[NUM_ALGORITHMS] = {
    LimiterAlgorithm::Transparent,
    LimiterAlgorithm::Punchy,
    LimiterAlgorithm::Dynamic,
    LimiterAlgorithm::Aggressive,
    LimiterAlgorithm::Allround,
    LimiterAlgorithm::Bus,
    LimiterAlgorithm::Safe,
    LimiterAlgorithm::Modern
};

TEST_CASE ("test_all_algorithms_have_params", "[LimiterAlgorithm]")
{
    for (int i = 0; i < NUM_ALGORITHMS; ++i)
    {
        AlgorithmParams p = getAlgorithmParams (allAlgorithms[i]);
        REQUIRE (std::isfinite (p.kneeWidth));
        REQUIRE (p.kneeWidth >= 0.0f);
        REQUIRE (p.kneeWidth <= 12.0f);
        REQUIRE (std::isfinite (p.saturationAmount));
        REQUIRE (p.saturationAmount >= 0.0f);
        REQUIRE (p.saturationAmount <= 1.0f);
        REQUIRE (std::isfinite (p.transientAttackCoeff));
        REQUIRE (p.transientAttackCoeff >= 0.0f);
        REQUIRE (p.transientAttackCoeff <= 1.0f);
        REQUIRE (std::isfinite (p.releaseShape));
    }
}

TEST_CASE ("test_algorithm_params_ranges", "[LimiterAlgorithm]")
{
    for (int i = 0; i < NUM_ALGORITHMS; ++i)
    {
        AlgorithmParams p = getAlgorithmParams (allAlgorithms[i]);

        REQUIRE (p.transientAttackCoeff >= 0.0f);
        REQUIRE (p.transientAttackCoeff <= 1.0f);

        REQUIRE (p.releaseShape >= 0.0f);
        REQUIRE (p.releaseShape <= 1.0f);

        REQUIRE (p.saturationAmount >= 0.0f);
        REQUIRE (p.saturationAmount <= 1.0f);

        REQUIRE (p.kneeWidth >= 0.0f);
        REQUIRE (p.kneeWidth <= 12.0f);
    }
}

TEST_CASE ("test_algorithm_enum_count", "[LimiterAlgorithm]")
{
    // Verify all 8 algorithms are distinct (different enough to be unique)
    // Check that all algorithms have valid params
    REQUIRE (static_cast<int> (LimiterAlgorithm::Transparent) == 0);
    REQUIRE (static_cast<int> (LimiterAlgorithm::Punchy)      == 1);
    REQUIRE (static_cast<int> (LimiterAlgorithm::Dynamic)     == 2);
    REQUIRE (static_cast<int> (LimiterAlgorithm::Aggressive)  == 3);
    REQUIRE (static_cast<int> (LimiterAlgorithm::Allround)    == 4);
    REQUIRE (static_cast<int> (LimiterAlgorithm::Bus)         == 5);
    REQUIRE (static_cast<int> (LimiterAlgorithm::Safe)        == 6);
    REQUIRE (static_cast<int> (LimiterAlgorithm::Modern)      == 7);
}

TEST_CASE ("test_safe_has_zero_saturation", "[LimiterAlgorithm]")
{
    AlgorithmParams p = getAlgorithmParams (LimiterAlgorithm::Safe);
    REQUIRE (p.saturationAmount == Catch::Approx (0.0f));
}

TEST_CASE ("test_safe_has_widest_knee", "[LimiterAlgorithm]")
{
    AlgorithmParams safe = getAlgorithmParams (LimiterAlgorithm::Safe);
    for (int i = 0; i < NUM_ALGORITHMS; ++i)
    {
        if (allAlgorithms[i] == LimiterAlgorithm::Safe)
            continue;
        AlgorithmParams other = getAlgorithmParams (allAlgorithms[i]);
        REQUIRE (safe.kneeWidth >= other.kneeWidth);
    }
}

TEST_CASE ("test_transparent_has_zero_saturation", "[LimiterAlgorithm]")
{
    AlgorithmParams p = getAlgorithmParams (LimiterAlgorithm::Transparent);
    REQUIRE (p.saturationAmount == Catch::Approx (0.0f));
    REQUIRE (p.adaptiveRelease == true);
}

TEST_CASE ("test_aggressive_has_high_saturation", "[LimiterAlgorithm]")
{
    AlgorithmParams p = getAlgorithmParams (LimiterAlgorithm::Aggressive);
    REQUIRE (p.saturationAmount >= 0.5f);
    REQUIRE (p.transientAttackCoeff >= 0.8f);
}

// ---------------------------------------------------------------------------
// Helpers for audio-output distinctiveness tests
// ---------------------------------------------------------------------------
static constexpr double kAlgoSampleRate  = 44100.0;
static constexpr int    kAlgoBlockSize   = 1024;
static constexpr int    kAlgoNumChannels = 2;
static constexpr int    kWarmupBlocks    = 3;      // blocks to reach steady state

/** Fill all channels with a constant amplitude value. */
static void fillConstantAlgo (std::vector<std::vector<float>>& buf, float value)
{
    for (auto& ch : buf)
        std::fill (ch.begin(), ch.end(), value);
}

/** Fill all channels with a 440 Hz sine at the given peak amplitude. */
static void fillSineAlgo (std::vector<std::vector<float>>& buf, float peakAmp,
                          int startSample)
{
    const int n = static_cast<int> (buf[0].size());
    for (auto& ch : buf)
    {
        for (int i = 0; i < n; ++i)
        {
            const double t = (startSample + i) / kAlgoSampleRate;
            ch[static_cast<size_t> (i)] =
                peakAmp * static_cast<float> (std::sin (2.0 * 3.14159265358979323846 * 440.0 * t));
        }
    }
}

/** Peak of a single channel buffer. */
static float peakOf (const std::vector<float>& data)
{
    float peak = 0.0f;
    for (float v : data)
        peak = std::max (peak, std::abs (v));
    return peak;
}

/** RMS of a single channel buffer. */
static float rmsOf (const std::vector<float>& data)
{
    double sum = 0.0;
    for (float v : data)
        sum += static_cast<double> (v) * v;
    return static_cast<float> (std::sqrt (sum / data.size()));
}

/** Build raw pointer array from vector-of-vectors. */
static std::vector<float*> makePtrsAlgo (std::vector<std::vector<float>>& buf)
{
    std::vector<float*> ptrs (buf.size());
    for (size_t i = 0; i < buf.size(); ++i)
        ptrs[i] = buf[i].data();
    return ptrs;
}

/**
 * Process DC audio at the given amplitude through TransientLimiter + LevelingLimiter
 * with the given algorithm. Returns the output peak of the last block on channel 0.
 *
 * Uses 1.3f amplitude by default, which places wide-knee algorithms (Transparent,
 * Bus, Safe with knee ≥ 6 dB) inside their soft knee — yielding partial gain
 * reduction — while narrow-knee algorithms (Aggressive, Punchy, Dynamic, Allround,
 * Modern) are above their knee and apply full threshold-clamping, followed by
 * their respective saturation amounts. This makes all 8 algorithms distinguishable.
 */
static float measureOutputPeakDC (LimiterAlgorithm algo, float inputAmp = 1.3f)
{
    TransientLimiter tl;
    LevelingLimiter  ll;
    tl.prepare (kAlgoSampleRate, kAlgoBlockSize, kAlgoNumChannels);
    ll.prepare (kAlgoSampleRate, kAlgoBlockSize, kAlgoNumChannels);

    const AlgorithmParams params = getAlgorithmParams (algo);
    tl.setAlgorithmParams (params);
    ll.setAlgorithmParams (params);
    tl.setLookahead (1.0f);

    std::vector<std::vector<float>> buf (kAlgoNumChannels,
                                         std::vector<float> (kAlgoBlockSize));

    for (int b = 0; b < kWarmupBlocks; ++b)
    {
        fillConstantAlgo (buf, inputAmp);
        auto ptrs = makePtrsAlgo (buf);
        tl.process (ptrs.data(), kAlgoNumChannels, kAlgoBlockSize);
        ll.process (ptrs.data(), kAlgoNumChannels, kAlgoBlockSize);
    }

    fillConstantAlgo (buf, inputAmp);
    auto ptrs = makePtrsAlgo (buf);
    tl.process (ptrs.data(), kAlgoNumChannels, kAlgoBlockSize);
    ll.process (ptrs.data(), kAlgoNumChannels, kAlgoBlockSize);

    return peakOf (buf[0]);
}

/**
 * Process a 440 Hz sine at peakAmp=2.0f through both limiter stages and
 * return the output peak and RMS of the last block on channel 0.
 *
 * Saturation (tanh waveshaping) compresses peaks more than zero-crossings,
 * changing the peak-to-RMS ratio. A constant DC signal cannot show this
 * effect (peak == RMS for DC), so a sine is required.
 */
static std::pair<float,float> measureSineOutputPeakAndRMS (LimiterAlgorithm algo,
                                                            float peakAmp = 2.0f)
{
    TransientLimiter tl;
    LevelingLimiter  ll;
    tl.prepare (kAlgoSampleRate, kAlgoBlockSize, kAlgoNumChannels);
    ll.prepare (kAlgoSampleRate, kAlgoBlockSize, kAlgoNumChannels);

    const AlgorithmParams params = getAlgorithmParams (algo);
    tl.setAlgorithmParams (params);
    ll.setAlgorithmParams (params);
    tl.setLookahead (1.0f);

    std::vector<std::vector<float>> buf (kAlgoNumChannels,
                                         std::vector<float> (kAlgoBlockSize));

    int samplePos = 0;
    for (int b = 0; b < kWarmupBlocks; ++b)
    {
        fillSineAlgo (buf, peakAmp, samplePos);
        samplePos += kAlgoBlockSize;
        auto ptrs = makePtrsAlgo (buf);
        tl.process (ptrs.data(), kAlgoNumChannels, kAlgoBlockSize);
        ll.process (ptrs.data(), kAlgoNumChannels, kAlgoBlockSize);
    }

    fillSineAlgo (buf, peakAmp, samplePos);
    auto ptrs = makePtrsAlgo (buf);
    tl.process (ptrs.data(), kAlgoNumChannels, kAlgoBlockSize);
    ll.process (ptrs.data(), kAlgoNumChannels, kAlgoBlockSize);

    return { peakOf (buf[0]), rmsOf (buf[0]) };
}

// ---------------------------------------------------------------------------
// test_all_algorithms_produce_distinct_gr
//
// Each of the 8 algorithms has different DSP parameters (knee, saturation,
// attack, release shape). To expose all differences with a constant DC signal
// we use 1.3f (+2.28 dBFS) — a level that sits inside the soft knees of the
// wide-knee algorithms (Transparent knee=6, Bus knee=6, Safe knee=12) so
// they apply only partial gain reduction, while the narrow-knee algorithms
// (Aggressive, Punchy, Dynamic, Allround, Modern) are fully above their
// knees and limit hard to threshold before their respective saturation stages
// produce distinct final amplitudes.
//
// Any accidental duplication of a case in getAlgorithmParams() would yield
// an output peak within 0.001 linear of another algorithm and be caught here.
// ---------------------------------------------------------------------------
TEST_CASE ("test_all_algorithms_produce_distinct_gr", "[LimiterAlgorithm]")
{
    static const LimiterAlgorithm all8[NUM_ALGORITHMS] = {
        LimiterAlgorithm::Transparent,
        LimiterAlgorithm::Punchy,
        LimiterAlgorithm::Dynamic,
        LimiterAlgorithm::Aggressive,
        LimiterAlgorithm::Allround,
        LimiterAlgorithm::Bus,
        LimiterAlgorithm::Safe,
        LimiterAlgorithm::Modern
    };

    float outputPeaks[NUM_ALGORITHMS];
    for (int i = 0; i < NUM_ALGORITHMS; ++i)
        outputPeaks[i] = measureOutputPeakDC (all8[i], 1.3f);

    // Distinct threshold: 0.001 linear ≈ 0.008 dB — any two algorithms
    // whose output peaks are closer than this are considered duplicates.
    static constexpr float kDistinctTolerance = 0.001f;

    for (int i = 0; i < NUM_ALGORITHMS; ++i)
    {
        for (int j = i + 1; j < NUM_ALGORITHMS; ++j)
        {
            const float diff = std::abs (outputPeaks[i] - outputPeaks[j]);
            INFO ("Algorithms " << i << " and " << j
                  << " output peaks: " << outputPeaks[i] << " vs " << outputPeaks[j]
                  << " (diff=" << diff << ")");
            CHECK (diff > kDistinctTolerance);
        }
    }
}

// ---------------------------------------------------------------------------
// test_aggressive_more_gr_than_safe
//
// Aggressive (hard knee=0.5, sat=0.8) applies full threshold clamping then
// heavy tanh saturation at +6 dBFS, driving its output well below 1.0.
// Safe (wide knee=12, sat=0) applies only partial gain reduction at +6 dBFS
// (it is still inside its soft knee), resulting in an output > 1.0 (or
// at minimum, clearly higher than Aggressive's).
// More GR = lower output peak.
// ---------------------------------------------------------------------------
TEST_CASE ("test_aggressive_more_gr_than_safe", "[LimiterAlgorithm]")
{
    // Use 2.0f (+6 dBFS): Aggressive limits hard and saturates heavily,
    // Safe also limits but with no saturation. Aggressive always has a
    // lower output peak at this amplitude.
    const float aggressivePeak = measureOutputPeakDC (LimiterAlgorithm::Aggressive, 2.0f);
    const float safePeak       = measureOutputPeakDC (LimiterAlgorithm::Safe,       2.0f);

    INFO ("Aggressive output peak: " << aggressivePeak);
    INFO ("Safe output peak:       " << safePeak);

    // Aggressive must produce a lower output peak (more effective GR) than Safe.
    CHECK (aggressivePeak < safePeak);
}

// ---------------------------------------------------------------------------
// test_saturation_affects_waveform_shape
//
// Transparent (saturationAmount=0) does not modify the limited waveform.
// Aggressive (saturationAmount=0.8) applies tanh waveshaping which compresses
// peaks more than zero-crossings, reducing the peak-to-RMS (crest factor).
//
// A constant DC signal cannot reveal this (peak == RMS == constant), so we
// feed a 440 Hz sine at 2.0f peak. After limiting, the output peaks are at
// or below threshold while the near-zero regions are barely affected. Tanh
// saturation in Aggressive further suppresses the peaks (tanh(1) ≈ 0.76)
// while leaving the small-amplitude regions nearly unchanged (tanh(x) ≈ x
// for small x). This produces a measurably lower peak-to-RMS ratio for
// Aggressive vs Transparent.
// ---------------------------------------------------------------------------
TEST_CASE ("test_saturation_affects_waveform_shape", "[LimiterAlgorithm]")
{
    auto [tPeak, tRMS] = measureSineOutputPeakAndRMS (LimiterAlgorithm::Transparent);
    auto [aPeak, aRMS] = measureSineOutputPeakAndRMS (LimiterAlgorithm::Aggressive);

    INFO ("Transparent peak=" << tPeak << " rms=" << tRMS
          << " ratio=" << (tRMS > 0.0f ? tPeak / tRMS : 0.0f));
    INFO ("Aggressive  peak=" << aPeak << " rms=" << aRMS
          << " ratio=" << (aRMS > 0.0f ? aPeak / aRMS : 0.0f));

    REQUIRE (tRMS > 0.0f);
    REQUIRE (aRMS > 0.0f);

    const float transparentRatio = tPeak / tRMS;
    const float aggressiveRatio  = aPeak / aRMS;

    // Saturation in Aggressive compresses peaks relative to RMS:
    // the crest factor is lower for Aggressive than for Transparent.
    // The two ratios must differ by a measurable amount (> 0.01).
    const float ratioDiff = std::abs (transparentRatio - aggressiveRatio);
    INFO ("Crest-factor difference: " << ratioDiff);
    CHECK (ratioDiff > 0.01f);
}

// ---------------------------------------------------------------------------
// test_transient_attack_coeff_affects_gr_onset
//
// A TransientLimiter with transientAttackCoeff=0.95 (fast) must apply gain
// reduction within fewer samples of a sudden peak onset than one with
// transientAttackCoeff=0.2 (slow). We feed a block of silence followed by
// a full-amplitude pulse and compare gain states after just a few samples of
// the transient, confirming the fast instance reduces gain more quickly.
// ---------------------------------------------------------------------------
TEST_CASE ("test_transient_attack_coeff_affects_gr_onset", "[LimiterAlgorithm][TransientLimiter]")
{
    constexpr double sr         = 44100.0;
    constexpr int    blockSize  = 64;
    constexpr int    numCh      = 1;
    constexpr float  kThreshold = 1.0f;
    constexpr float  kPeakAmp   = 2.0f;   // well above threshold

    // Build two limiters: slow attack vs fast attack.
    TransientLimiter slow, fast;
    slow.prepare (sr, blockSize, numCh);
    fast.prepare (sr, blockSize, numCh);
    slow.setThreshold (kThreshold);
    fast.setThreshold (kThreshold);

    // No lookahead so gain reduction happens in the same block as the peak.
    slow.setLookahead (0.0f);
    fast.setLookahead (0.0f);

    AlgorithmParams slowParams{};
    slowParams.transientAttackCoeff = 0.2f;
    slowParams.releaseShape         = 0.5f;
    slowParams.saturationAmount     = 0.0f;
    slowParams.kneeWidth            = 0.0f;
    slowParams.adaptiveRelease      = false;

    AlgorithmParams fastParams = slowParams;
    fastParams.transientAttackCoeff = 0.95f;

    slow.setAlgorithmParams (slowParams);
    fast.setAlgorithmParams (fastParams);

    // Process a single block: first 32 samples silence, last 32 samples at kPeakAmp.
    std::vector<float> slowBuf (blockSize, 0.0f);
    std::vector<float> fastBuf (blockSize, 0.0f);
    for (int i = blockSize / 2; i < blockSize; ++i)
    {
        slowBuf[static_cast<size_t>(i)] = kPeakAmp;
        fastBuf[static_cast<size_t>(i)] = kPeakAmp;
    }

    float* slowPtr = slowBuf.data();
    float* fastPtr = fastBuf.data();

    slow.process (&slowPtr, numCh, blockSize);
    fast.process (&fastPtr, numCh, blockSize);

    // After the onset (last 32 samples), the fast limiter should have applied
    // more gain reduction (lower output amplitude) than the slow limiter.
    float slowMaxGR = 0.0f;
    float fastMaxGR = 0.0f;
    for (int i = blockSize / 2; i < blockSize; ++i)
    {
        // GR = input - output (in amplitude); larger = more reduction
        const float input = kPeakAmp;
        slowMaxGR = std::max (slowMaxGR, input - std::abs (slowBuf[static_cast<size_t>(i)]));
        fastMaxGR = std::max (fastMaxGR, input - std::abs (fastBuf[static_cast<size_t>(i)]));
    }

    INFO ("Slow-attack max GR amplitude: " << slowMaxGR);
    INFO ("Fast-attack max GR amplitude: " << fastMaxGR);

    // Fast coefficient must produce more gain reduction during onset.
    CHECK (fastMaxGR > slowMaxGR);
}

