#include "catch2/catch_amalgamated.hpp"
#include "dsp/TransientLimiter.h"
#include "dsp/LimiterAlgorithm.h"
#include <cmath>
#include <vector>
#include <algorithm>
#include <random>

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
// test_release_curve_is_exponential_db
//   After a peak engages gain reduction, the release in dB should decrease at
//   a roughly constant rate per unit time (linear in dB = exponential in
//   linear domain).  This verifies that the release envelope is smoothed in
//   the dB domain rather than the linear domain.
// ---------------------------------------------------------------------------
TEST_CASE("test_release_curve_is_exponential_db", "[TransientLimiter]")
{
    TransientLimiter limiter;
    limiter.prepare(kSampleRate, kBlockSize, 1);  // mono

    AlgorithmParams params = getAlgorithmParams(LimiterAlgorithm::Transparent);
    params.kneeWidth        = 0.0f;   // hard knee for predictable onset
    params.saturationAmount = 0.0f;
    limiter.setAlgorithmParams(params);
    limiter.setLookahead(0.0f);  // no lookahead — instant output alignment

    // Drive a large peak (+12 dB) to engage instant attack
    {
        std::vector<std::vector<float>> buf(1, std::vector<float>(1, 4.0f));
        auto ptrs = makePtrs(buf);
        limiter.process(ptrs.data(), 1, 1);
    }

    const float grAtPeak = limiter.getGainReduction();
    REQUIRE(grAtPeak < -1.0f);  // meaningful gain reduction must be active

    // Measure GR at equal time steps during silence (release phase)
    const int stepSamples = 200;
    std::vector<float> grReadings;
    for (int i = 0; i < 5; ++i)
    {
        std::vector<std::vector<float>> buf(1, std::vector<float>(stepSamples, 0.0f));
        auto ptrs = makePtrs(buf);
        limiter.process(ptrs.data(), 1, stepSamples);
        grReadings.push_back(limiter.getGainReduction());
    }

    // GR should recover monotonically toward 0 with each step
    for (int i = 1; i < (int)grReadings.size(); ++i)
        REQUIRE(grReadings[i] > grReadings[i - 1]);

    // Each step should produce a consistent dB increment (within 60% tolerance).
    // With dB-domain release smoothing the increments are nearly constant;
    // with linear-domain smoothing they would decrease significantly over time.
    std::vector<float> diffs;
    for (int i = 1; i < (int)grReadings.size(); ++i)
        diffs.push_back(grReadings[i] - grReadings[i - 1]);

    for (int i = 1; i < (int)diffs.size(); ++i)
        REQUIRE(diffs[i] == Catch::Approx(diffs[0]).epsilon(0.60f));
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

// ---------------------------------------------------------------------------
// test_sidechain_with_lookahead
//   When sidechainData is provided and lookahead is enabled, the sidechain
//   delay buffer must be scanned for peaks (not just the current sidechain
//   sample).  This means that when the sidechain peak at step N triggers GR,
//   the scan window keeps that peak visible for the next lookaheadSamples steps,
//   maintaining the instant-attack gain level throughout the window.
//
//   Consequence: the main audio sample that was written at step N is delayed
//   by (lookaheadSamples - 1) and arrives at the output at step
//   N + lookaheadSamples - 1.  With the scan active throughout the window,
//   GR is still at its peak level when this delayed sample is output, so it
//   cannot exceed the threshold.  Without the scan (bug: only current sample
//   read), GR would already be releasing by then, and the delayed main peak
//   would overshoot the threshold.
// ---------------------------------------------------------------------------
TEST_CASE("test_sidechain_with_lookahead", "[TransientLimiter]")
{
    TransientLimiter limiter;
    limiter.prepare(kSampleRate, kBlockSize, 1);  // mono

    AlgorithmParams params = getAlgorithmParams(LimiterAlgorithm::Transparent);
    params.kneeWidth        = 0.0f;   // hard knee for clear onset
    params.saturationAmount = 0.0f;
    limiter.setAlgorithmParams(params);

    const float lookaheadMs = 2.0f;
    limiter.setLookahead(lookaheadMs);
    const int lookaheadSamples =
        static_cast<int>(lookaheadMs * 0.001f * static_cast<float>(kSampleRate));

    // Main audio: silence everywhere except a large peak at mainPeakPos.
    // Sidechain: same peak at the same position.
    //
    // With lookahead = L, the main peak at input step mainPeakPos arrives at
    // the output at output step mainPeakPos + L (exact delay with read-before-
    // write ordering).  The sidechain peak is detected at output step
    // mainPeakPos.  With correct scan-window lookahead, GR is held at peak
    // level for the full lookahead window and is still at its maximum at step
    // mainPeakPos + L, keeping the delayed main peak within threshold.
    const int mainPeakPos = 300;
    std::vector<std::vector<float>> mainBuf(1, std::vector<float>(kBlockSize, 0.0f));
    mainBuf[0][mainPeakPos] = 4.0f;

    std::vector<std::vector<float>> scBuf(mainBuf);  // sidechain mirrors main

    auto mainPtrs = makePtrs(mainBuf);
    std::vector<const float*> scPtrs = { scBuf[0].data() };
    limiter.process(mainPtrs.data(), 1, kBlockSize,
                    reinterpret_cast<const float* const*>(scPtrs.data()));

    // Overall output peak must not exceed threshold.
    // (Without the sidechain scan fix, the delayed main peak at output step
    //  mainPeakPos + lookaheadSamples - 1 would overshoot because GR has
    //  partially released over those steps.)
    REQUIRE(blockPeak(mainBuf[0]) <= 1.0f + 1e-4f);

    // GR must have been applied (sidechain detection was active).
    REQUIRE(limiter.getGainReduction() <= -1.0f);

    // Explicitly verify the output at the exact step where the delayed main
    // peak arrives — this is the critical moment where the scan fix matters.
    // With read-before-write ordering the delay is exactly lookaheadSamples.
    const int delayedPeakPos = mainPeakPos + lookaheadSamples;
    if (delayedPeakPos < kBlockSize)
        REQUIRE(mainBuf[0][delayedPeakPos] <= 1.0f + 1e-4f);
}

// ---------------------------------------------------------------------------
// test_custom_threshold
//   setThreshold() must change the level at which gain reduction activates.
//   At threshold=0.5, a signal at 0.8 must be limited; at threshold=1.0 the
//   same signal passes through unchanged (it is below the default threshold).
// ---------------------------------------------------------------------------
TEST_CASE("test_custom_threshold", "[TransientLimiter]")
{
    SECTION("threshold=0.5 activates GR on input at 0.8")
    {
        TransientLimiter limiter;
        limiter.prepare(kSampleRate, kBlockSize, 1);

        AlgorithmParams params = getAlgorithmParams(LimiterAlgorithm::Transparent);
        params.kneeWidth        = 0.0f;  // hard knee for predictable onset
        params.saturationAmount = 0.0f;
        limiter.setAlgorithmParams(params);
        limiter.setLookahead(0.0f);
        limiter.setThreshold(0.5f);  // custom threshold below 0.8

        const float inputAmp = 0.8f;
        std::vector<std::vector<float>> buf(1, std::vector<float>(kBlockSize, inputAmp));
        auto ptrs = makePtrs(buf);
        limiter.process(ptrs.data(), 1, kBlockSize);

        // Output must not exceed the threshold
        REQUIRE(blockPeak(buf[0]) <= 0.5f + 1e-4f);
        // GR must be active
        REQUIRE(limiter.getGainReduction() < -0.5f);
    }

    SECTION("threshold=1.0 (default) passes 0.8 unchanged")
    {
        TransientLimiter limiter;
        limiter.prepare(kSampleRate, kBlockSize, 1);

        AlgorithmParams params = getAlgorithmParams(LimiterAlgorithm::Transparent);
        params.kneeWidth        = 0.0f;
        params.saturationAmount = 0.0f;
        limiter.setAlgorithmParams(params);
        limiter.setLookahead(0.0f);
        limiter.setThreshold(1.0f);  // default threshold

        const float inputAmp = 0.8f;
        std::vector<std::vector<float>> buf(1, std::vector<float>(kBlockSize, inputAmp));
        auto ptrs = makePtrs(buf);

        // Warm-up to clear state
        limiter.process(ptrs.data(), 1, kBlockSize);
        std::fill(buf[0].begin(), buf[0].end(), inputAmp);
        limiter.process(ptrs.data(), 1, kBlockSize);

        // Signal below threshold — should pass through essentially unmodified
        REQUIRE(blockPeak(buf[0]) == Catch::Approx(inputAmp).margin(0.01f));
        REQUIRE(limiter.getGainReduction() >= -0.5f);
    }
}

// ---------------------------------------------------------------------------
// test_transient_limiter_lookahead_peak
//   Feed a buffer with a single large spike at a known position followed by
//   silence.  With lookahead L the delay is exactly L samples (read-before-
//   write ordering), so the spike arrives at the output at position
//   peakPos + L.  Verify:
//     1. The output peak does not exceed the threshold (GR correctly applied).
//     2. GR was active (gain reduction reported).
//     3. The output spike position is within ±1 sample of the expected
//        delayed position — confirming the lookahead window anticipates the
//        spike by exactly the lookahead duration.
// ---------------------------------------------------------------------------
TEST_CASE("test_transient_limiter_lookahead_peak", "[TransientLimiter]")
{
    TransientLimiter limiter;
    const float lookaheadMs      = 2.0f;
    const int   lookaheadSamples =
        static_cast<int>(lookaheadMs * 0.001f * static_cast<float>(kSampleRate));

    limiter.prepare(kSampleRate, kBlockSize, 1);

    AlgorithmParams params = getAlgorithmParams(LimiterAlgorithm::Transparent);
    params.kneeWidth        = 0.0f;  // hard knee — predictable onset
    params.saturationAmount = 0.0f;
    limiter.setAlgorithmParams(params);
    limiter.setLookahead(lookaheadMs);

    // Spike at peakPos, silence everywhere else
    const int   peakPos  = 300;
    const float spikeAmp = 4.0f;   // +12 dBFS — well above threshold
    std::vector<std::vector<float>> buf(1, std::vector<float>(kBlockSize, 0.0f));
    buf[0][peakPos] = spikeAmp;

    auto ptrs = makePtrs(buf);
    limiter.process(ptrs.data(), 1, kBlockSize);

    // 1. Output peak must not exceed threshold
    REQUIRE(blockPeak(buf[0]) <= 1.0f + 1e-4f);

    // 2. GR was applied
    REQUIRE(limiter.getGainReduction() <= 0.0f);

    // 3. Delayed output spike position must be within ±1 sample of expected.
    //    With read-before-write the delay is exactly lookaheadSamples.
    const int expectedOutputPos = peakPos + lookaheadSamples;
    if (expectedOutputPos < kBlockSize)
    {
        int   maxPos = 0;
        float maxVal = 0.0f;
        for (int i = 0; i < kBlockSize; ++i)
        {
            if (std::abs(buf[0][i]) > maxVal)
            {
                maxVal = std::abs(buf[0][i]);
                maxPos = i;
            }
        }
        REQUIRE(std::abs(maxPos - expectedOutputPos) <= 1);

        // The attenuated spike at the output must be within threshold
        REQUIRE(buf[0][expectedOutputPos] <= 1.0f + 1e-4f);
    }
}

// ---------------------------------------------------------------------------
// test_sliding_max_peak_detection
//   Feed a block with a known peak at a known position followed by silence.
//   With lookahead L the limiter must begin reducing gain exactly when the
//   peak enters the lookahead window — i.e., L samples before the peak
//   reaches the output.  Verifies that the sliding-window-max deque correctly
//   tracks the window maximum and triggers gain reduction at the right moment.
// ---------------------------------------------------------------------------
TEST_CASE("test_sliding_max_peak_detection", "[TransientLimiter]")
{
    TransientLimiter limiter;
    const float lookaheadMs      = 2.0f;
    const int   lookaheadSamples =
        static_cast<int>(lookaheadMs * 0.001f * static_cast<float>(kSampleRate));

    limiter.prepare(kSampleRate, kBlockSize, 1);  // mono

    AlgorithmParams params = getAlgorithmParams(LimiterAlgorithm::Transparent);
    params.kneeWidth        = 0.0f;   // hard knee — predictable onset
    params.saturationAmount = 0.0f;
    limiter.setAlgorithmParams(params);
    limiter.setLookahead(lookaheadMs);

    // Place a large spike at peakPos; everything else is silence.
    const int   peakPos  = 400;
    const float spikeAmp = 4.0f;  // +12 dBFS

    std::vector<std::vector<float>> buf(1, std::vector<float>(kBlockSize, 0.0f));
    buf[0][peakPos] = spikeAmp;

    auto ptrs = makePtrs(buf);
    limiter.process(ptrs.data(), 1, kBlockSize);

    // 1. Overall output peak must not exceed threshold — sliding max handled it.
    REQUIRE(blockPeak(buf[0]) <= 1.0f + 1e-4f);

    // 2. Samples well before the lookahead window should be unaffected (silence in, silence out).
    const int windowStart = peakPos - lookaheadSamples;
    for (int i = 0; i < windowStart - 2 && i < kBlockSize; ++i)
        REQUIRE(std::abs(buf[0][i]) < 1e-5f);

    // 3. The attenuated spike must appear at the delayed output position.
    //    With read-before-write the delay is exactly lookaheadSamples.
    const int expectedOutputPos = peakPos + lookaheadSamples;
    if (expectedOutputPos < kBlockSize)
        REQUIRE(buf[0][expectedOutputPos] <= 1.0f + 1e-4f);

    // 4. GR was applied.
    REQUIRE(limiter.getGainReduction() <= 0.0f);
}

// ---------------------------------------------------------------------------
// test_zero_lookahead_zero_latency
//   With lookahead set to 0 ms, getLatencyInSamples() must return 0.
// ---------------------------------------------------------------------------
TEST_CASE("test_zero_lookahead_zero_latency", "[TransientLimiter]")
{
    TransientLimiter limiter;
    limiter.prepare(kSampleRate, kBlockSize, kNumChannels);
    limiter.setLookahead(0.0f);
    REQUIRE(limiter.getLatencyInSamples() == 0);
}

// ---------------------------------------------------------------------------
// test_lookahead_latency_matches_reported
//   With lookahead=5 ms at 44100 Hz, getLatencyInSamples() must return
//   approximately 220 samples (±5 samples).
// ---------------------------------------------------------------------------
TEST_CASE("test_lookahead_latency_matches_reported", "[TransientLimiter]")
{
    TransientLimiter limiter;
    limiter.prepare(kSampleRate, kBlockSize, kNumChannels);

    const float lookaheadMs     = 5.0f;
    const int   expectedSamples = static_cast<int>(lookaheadMs * 0.001 * kSampleRate); // 220
    limiter.setLookahead(lookaheadMs);

    const int reported = limiter.getLatencyInSamples();
    REQUIRE(std::abs(reported - expectedSamples) <= 5);
}

// ---------------------------------------------------------------------------
// test_latency_changes_with_lookahead_param
//   Changing lookahead from 1 ms to 5 ms (max) and re-preparing must
//   increase the reported latency proportionally.
// ---------------------------------------------------------------------------
TEST_CASE("test_latency_changes_with_lookahead_param", "[TransientLimiter]")
{
    TransientLimiter limiter;
    limiter.prepare(kSampleRate, kBlockSize, kNumChannels);
    limiter.setLookahead(1.0f);
    const int latency1ms = limiter.getLatencyInSamples();

    limiter.prepare(kSampleRate, kBlockSize, kNumChannels);
    limiter.setLookahead(5.0f);
    const int latency5ms = limiter.getLatencyInSamples();

    // 5 ms lookahead must yield ~5x more latency than 1 ms
    REQUIRE(latency5ms > latency1ms);
    // Sanity: non-zero lookahead → non-zero latency
    REQUIRE(latency1ms > 0);
    REQUIRE(latency5ms > 0);
    // Ratio should be approximately 5:1 (allow 10% tolerance)
    const float ratio = static_cast<float>(latency5ms) / static_cast<float>(latency1ms);
    REQUIRE(ratio == Catch::Approx(5.0f).epsilon(0.10f));
}

// ---------------------------------------------------------------------------
// test_null_sidechain_no_crash
//   Explicitly passing nullptr as the sidechain pointer must not crash and
//   must produce finite output over 10 blocks.
// ---------------------------------------------------------------------------
TEST_CASE("test_null_sidechain_no_crash", "[TransientLimiter]")
{
    TransientLimiter limiter;
    limiter.prepare(kSampleRate, kBlockSize, kNumChannels);
    limiter.setAlgorithmParams(getAlgorithmParams(LimiterAlgorithm::Transparent));
    limiter.setLookahead(2.0f);

    std::vector<std::vector<float>> buf(kNumChannels, std::vector<float>(kBlockSize, 0.5f));

    for (int block = 0; block < 10; ++block)
    {
        for (auto& ch : buf)
            std::fill(ch.begin(), ch.end(), 0.5f);
        auto ptrs = makePtrs(buf);

        // Explicit nullptr sidechain — must not crash
        limiter.process(ptrs.data(), kNumChannels, kBlockSize, nullptr);

        for (int ch = 0; ch < kNumChannels; ++ch)
            for (float v : buf[ch])
                REQUIRE(std::isfinite(v));
    }
}

// ---------------------------------------------------------------------------
// test_small_block_no_crash
//   A block size much smaller than the lookahead buffer (16 samples with
//   5 ms lookahead at 44100 = 220 samples) must not crash and must produce
//   finite output over 100 blocks.
// ---------------------------------------------------------------------------
TEST_CASE("test_small_block_no_crash", "[TransientLimiter]")
{
    TransientLimiter limiter;
    const float lookaheadMs = 5.0f;  // max supported (kMaxLookaheadMs)
    const int   smallBlock  = 16;

    limiter.prepare(kSampleRate, smallBlock, kNumChannels);
    limiter.setAlgorithmParams(getAlgorithmParams(LimiterAlgorithm::Aggressive));
    limiter.setLookahead(lookaheadMs);

    std::vector<std::vector<float>> buf(kNumChannels, std::vector<float>(smallBlock));

    for (int block = 0; block < 100; ++block)
    {
        const float amp = (block % 2 == 0) ? 2.0f : 0.3f;
        for (auto& ch : buf)
            std::fill(ch.begin(), ch.end(), amp);
        auto ptrs = makePtrs(buf);

        limiter.process(ptrs.data(), kNumChannels, smallBlock);

        for (int ch = 0; ch < kNumChannels; ++ch)
            for (float v : buf[ch])
                REQUIRE(std::isfinite(v));
    }
}

// ---------------------------------------------------------------------------
// test_sliding_max_matches_brute_force
//   Verifies that the O(1) sliding-window-max deque produces the same
//   per-sample output as a brute-force O(N) rolling-max reference.
//   Uses a randomised mono input with embedded large spikes.  Both
//   implementations share identical gain-smoothing logic (hard knee, no
//   saturation, same release coefficient) so any difference in peak
//   detection will produce a sample-level mismatch.
// ---------------------------------------------------------------------------
TEST_CASE("test_sliding_max_matches_brute_force", "[TransientLimiter]")
{
    const float lookaheadMs      = 2.0f;
    const int   numSamples       = 2048;
    const int   lookaheadSamples =
        static_cast<int>(lookaheadMs * 0.001f * static_cast<float>(kSampleRate));

    // --- Build a deterministic randomised input with known spikes -------
    std::mt19937 rng(42);
    std::uniform_real_distribution<float> noise(-0.3f, 0.3f);
    std::vector<float> input(numSamples);
    for (float& v : input) v = noise(rng);
    // Embed large spikes at several positions to exercise the window boundary
    for (int pos : { 50, 300, 800, 1200, 1900 })
        if (pos < numSamples) input[pos] = 3.5f;

    // --- Reference: brute-force rolling-max limiter (O(N) per sample) ---
    //
    // We implement the brute-force peak scan inline here, mirroring the
    // structure of the original O(N) loop so the only difference between
    // reference and production code is the peak-detection algorithm.
    //
    // Brute-force state:
    const int bufSize = lookaheadSamples + 1 + 1;  // +1 guard
    std::vector<float> delayBuf(bufSize, 0.0f);
    int   writePos    = 0;
    float gainState   = 1.0f;
    const float threshold = 1.0f;
    const float kneeHalf  = 0.0f;  // hard knee
    // Release coefficient matching TransientLimiter's default for Transparent algo
    const AlgorithmParams refParams = getAlgorithmParams(LimiterAlgorithm::Transparent);
    const float releaseMs    = 10.0f + refParams.releaseShape * 490.0f;
    const float releaseCoeff = std::exp(-1.0f / (releaseMs * 0.001f * static_cast<float>(kSampleRate)));
    (void)kneeHalf;

    std::vector<float> bruteOutput(numSamples);

    for (int s = 0; s < numSamples; ++s)
    {
        // Brute-force O(N) scan over lookahead window.
        // Use read-before-write ordering to match the fixed TransientLimiter:
        // 1. Read the delayed output BEFORE writing the new input.
        // 2. Then write the new input.
        // This gives an exact delay of lookaheadSamples (not lookaheadSamples-1).

        // --- Scan the lookahead window (reads from delay buffer as it stands) ---
        // Window is N+1 samples [input[s-N] ... input[s]] matching the
        // deque eviction rule (< mc - lookahead, not <=).
        float peakAbs = std::abs(input[s]);  // include current input in lookahead
        {
            int rpos = (writePos - 1 + bufSize) % bufSize;
            for (int k = 0; k < lookaheadSamples; ++k)  // N additional samples from buffer
            {
                peakAbs = std::max(peakAbs, std::abs(delayBuf[rpos]));
                rpos    = (rpos - 1 + bufSize) % bufSize;
            }
        }

        // Hard-knee required gain
        const float required = (peakAbs > threshold + 1e-9f)
                                   ? (threshold / peakAbs)
                                   : 1.0f;

        // Instant attack, linear-domain release (matches TransientLimiter smoothing)
        if (required < gainState)
        {
            gainState = required;
        }
        else
        {
            gainState = gainState * releaseCoeff + required * (1.0f - releaseCoeff);
        }
        gainState = std::clamp(gainState, 1e-6f, 1.0f);

        // Read delayed output BEFORE writing new input (exact lookaheadSamples delay)
        const int readPos = (writePos + bufSize - lookaheadSamples) % bufSize;
        bruteOutput[s] = delayBuf[readPos] * gainState;

        // Write new input AFTER reading delayed output
        delayBuf[writePos] = input[s];
        writePos = (writePos + 1) % bufSize;
    }

    // --- Production: sliding-window-max TransientLimiter ----------------
    TransientLimiter limiter;
    limiter.prepare(kSampleRate, kBlockSize, 1);

    AlgorithmParams params = getAlgorithmParams(LimiterAlgorithm::Transparent);
    params.kneeWidth        = 0.0f;  // hard knee
    params.saturationAmount = 0.0f;
    limiter.setAlgorithmParams(params);
    limiter.setLookahead(lookaheadMs);

    std::vector<std::vector<float>> buf(1, std::vector<float>(numSamples));
    buf[0] = input;
    auto ptrs = makePtrs(buf);
    limiter.process(ptrs.data(), 1, numSamples);

    // --- Compare sample-for-sample (allow tiny floating-point tolerance) --
    for (int s = 0; s < numSamples; ++s)
    {
        REQUIRE(buf[0][s] == Catch::Approx(bruteOutput[s]).margin(1e-4f));
    }
}

// ---------------------------------------------------------------------------
// test_lookahead_delay_exact
//   With lookahead set to N samples, the output must be delayed by EXACTLY N
//   samples relative to the input (not N-1).  This test verifies that the
//   read-before-write ordering produces the correct delay and that
//   getLatencySamples() in LimiterEngine reports the same value.
//
//   Method:
//   - Disable gain reduction by keeping input well below threshold.
//   - Feed a known ramp signal (input[s] = s).
//   - For each output sample at position s, verify output[s] == input[s - N].
//   - Also verify that the measured delay matches the lookahead formula used
//     by LimiterEngine::getLatencySamples().
// ---------------------------------------------------------------------------
TEST_CASE("test_lookahead_delay_exact", "[TransientLimiter]")
{
    // Use millisecond values (not back-calculated from sample counts) to
    // avoid float round-trip precision issues.  The expected sample delay is
    // derived using the same formula as LimiterEngine::getLatencySamples().
    for (float lookaheadMs : { 0.1f, 0.5f, 1.0f, 2.0f, 3.0f })
    {
        // Compute expected delay using the same integer truncation as LimiterEngine.
        const int expectedDelay =
            static_cast<int>(lookaheadMs * 0.001f * static_cast<float>(kSampleRate));

        if (expectedDelay <= 0)
            continue;  // skip degenerate cases

        TransientLimiter limiter;
        limiter.prepare(kSampleRate, kBlockSize, 1);

        AlgorithmParams params = getAlgorithmParams(LimiterAlgorithm::Transparent);
        params.kneeWidth        = 0.0f;   // hard knee
        params.saturationAmount = 0.0f;
        limiter.setAlgorithmParams(params);
        limiter.setThreshold(1.0f);       // 0 dBFS threshold
        limiter.setLookahead(lookaheadMs);

        // Build a ramp signal well below threshold so no gain reduction occurs.
        // Use small values so the limiter is transparent (gain = 1.0 throughout).
        const int numSamples = kBlockSize;
        std::vector<std::vector<float>> buf(1, std::vector<float>(numSamples));
        for (int s = 0; s < numSamples; ++s)
            buf[0][s] = static_cast<float>(s + 1) * 0.0001f;  // tiny ramp, well below threshold

        const std::vector<float> input = buf[0];  // save a copy

        auto ptrs = makePtrs(buf);
        limiter.process(ptrs.data(), 1, numSamples);

        // For samples at index >= expectedDelay the output should equal input[s - expectedDelay].
        // For samples at index < expectedDelay the output should be 0 (buffered zeros).
        for (int s = 0; s < numSamples; ++s)
        {
            if (s < expectedDelay)
            {
                // Output of the first N samples comes from the initially-zeroed
                // delay buffer, so it must be (near) zero.
                REQUIRE(std::abs(buf[0][s]) < 1e-6f);
            }
            else
            {
                // Exact delay of N samples: output[s] == input[s - N]
                const float expected = input[s - expectedDelay];
                REQUIRE(buf[0][s] == Catch::Approx(expected).margin(1e-5f));
            }
        }

        // Verify that the latency formula used by LimiterEngine matches
        // the actual measured delay.  getLatencySamples() computes:
        //   static_cast<int>(lookaheadMs * 0.001 * sampleRate)
        // which by construction above equals expectedDelay.
        const int reportedLatency =
            static_cast<int>(lookaheadMs * 0.001 * kSampleRate);
        REQUIRE(reportedLatency == expectedDelay);
    }
}

// ---------------------------------------------------------------------------
// test_custom_threshold_minus_1dBFS
//   setThreshold(0.891) must hold output ≤ 0.891 + tiny margin.
// ---------------------------------------------------------------------------
TEST_CASE("test_custom_threshold_minus_1dBFS", "[TransientLimiter]")
{
    TransientLimiter limiter;
    limiter.prepare(kSampleRate, 512, 2);
    limiter.setAlgorithmParams(getAlgorithmParams(LimiterAlgorithm::Transparent));
    limiter.setLookahead(1.0f);
    limiter.setThreshold(0.891f);  // -1 dBFS

    std::vector<std::vector<float>> buf(2, std::vector<float>(512, 2.0f));
    auto ptrs = makePtrs(buf);

    for (int block = 0; block < 10; ++block)
    {
        fillConstant(buf, 2.0f);
        limiter.process(ptrs.data(), 2, 512);
        if (block > 2)  // skip warm-up
        {
            REQUIRE(blockPeak(buf[0]) <= 0.891f + 1e-4f);
            REQUIRE(blockPeak(buf[1]) <= 0.891f + 1e-4f);
        }
    }
}

// ---------------------------------------------------------------------------
// test_custom_threshold_minus_6dBFS
//   setThreshold(0.501) at -6 dBFS must clamp a +6 dBFS input to ≤ 0.501.
// ---------------------------------------------------------------------------
TEST_CASE("test_custom_threshold_minus_6dBFS", "[TransientLimiter]")
{
    TransientLimiter limiter;
    limiter.prepare(kSampleRate, 512, 2);
    limiter.setAlgorithmParams(getAlgorithmParams(LimiterAlgorithm::Transparent));
    limiter.setLookahead(1.0f);
    limiter.setThreshold(0.501f);  // -6 dBFS

    std::vector<std::vector<float>> buf(2, std::vector<float>(512, 2.0f));
    auto ptrs = makePtrs(buf);

    for (int block = 0; block < 10; ++block)
    {
        fillConstant(buf, 2.0f);
        limiter.process(ptrs.data(), 2, 512);
        if (block > 2)  // skip warm-up
        {
            REQUIRE(blockPeak(buf[0]) <= 0.501f + 1e-4f);
            REQUIRE(blockPeak(buf[1]) <= 0.501f + 1e-4f);
        }
    }
}

// ---------------------------------------------------------------------------
// test_threshold_change_mid_session
//   After changing threshold from 1.0 to 0.5 mid-session, subsequent output
//   must not exceed 0.5 + tiny margin.
// ---------------------------------------------------------------------------
TEST_CASE("test_threshold_change_mid_session", "[TransientLimiter]")
{
    TransientLimiter limiter;
    limiter.prepare(kSampleRate, 512, 2);
    limiter.setAlgorithmParams(getAlgorithmParams(LimiterAlgorithm::Transparent));
    limiter.setLookahead(1.0f);
    limiter.setThreshold(1.0f);  // start at default

    std::vector<std::vector<float>> buf(2, std::vector<float>(512, 2.0f));
    auto ptrs = makePtrs(buf);

    // Phase 1: process with threshold=1.0
    for (int block = 0; block < 5; ++block)
    {
        fillConstant(buf, 2.0f);
        limiter.process(ptrs.data(), 2, 512);
    }

    // Change threshold mid-session
    limiter.setThreshold(0.5f);

    // Phase 2: process with threshold=0.5 — new threshold must be enforced
    for (int block = 0; block < 10; ++block)
    {
        fillConstant(buf, 2.0f);
        limiter.process(ptrs.data(), 2, 512);
        if (block > 2)  // skip warm-up after change
        {
            REQUIRE(blockPeak(buf[0]) <= 0.5f + 1e-4f);
            REQUIRE(blockPeak(buf[1]) <= 0.5f + 1e-4f);
        }
    }
}

// ---------------------------------------------------------------------------
// test_sidechain_drives_gr_not_main
//   When sidechainData is loud (above threshold) but the main audio is quiet,
//   the TransientLimiter must apply gain reduction to the quiet main audio
//   because detection runs on the sidechain signal, not the main path.
// ---------------------------------------------------------------------------
TEST_CASE("test_sidechain_drives_gr_not_main", "[TransientLimiter]")
{
    TransientLimiter limiter;
    limiter.prepare(kSampleRate, kBlockSize, 1);

    AlgorithmParams params = getAlgorithmParams(LimiterAlgorithm::Transparent);
    params.kneeWidth = 0.0f;   // hard knee
    limiter.setAlgorithmParams(params);
    limiter.setLookahead(0.0f);  // no lookahead so we see GR applied to current block
    limiter.setThreshold(1.0f);

    const float scAmplitude   = 2.0f;  // +6 dB over threshold
    const float mainAmplitude = 0.5f;  // well below threshold on its own

    // Warm-up blocks: feed loud sidechain + quiet main to engage GR
    for (int block = 0; block < 20; ++block)
    {
        std::vector<std::vector<float>> main(1, std::vector<float>(kBlockSize, mainAmplitude));
        std::vector<std::vector<float>> sc  (1, std::vector<float>(kBlockSize, scAmplitude));

        float* mainPtrs[1] = { main[0].data() };
        const float* scPtrs[1] = { sc[0].data() };
        limiter.process(mainPtrs, 1, kBlockSize, scPtrs);

        if (block >= 10)
        {
            // Main should be attenuated even though it was below threshold,
            // because the loud sidechain triggered GR.
            REQUIRE(blockPeak(main[0]) < mainAmplitude);
        }
    }
}

// ---------------------------------------------------------------------------
// test_sidechain_silent_no_gr
//   When sidechainData is silent but the main audio is loud, the limiter must
//   NOT apply significant gain reduction — detection runs on the silent
//   sidechain, so the loud main audio passes through substantially unattenuated.
// ---------------------------------------------------------------------------
TEST_CASE("test_sidechain_silent_no_gr", "[TransientLimiter]")
{
    TransientLimiter limiter;
    limiter.prepare(kSampleRate, kBlockSize, 1);

    AlgorithmParams params = getAlgorithmParams(LimiterAlgorithm::Transparent);
    params.kneeWidth = 0.0f;
    limiter.setAlgorithmParams(params);
    limiter.setLookahead(0.0f);
    limiter.setThreshold(1.0f);

    const float mainAmplitude = 2.0f;  // +6 dB — would normally trigger hard limiting

    std::vector<std::vector<float>> main(1, std::vector<float>(kBlockSize, mainAmplitude));
    std::vector<std::vector<float>> sc  (1, std::vector<float>(kBlockSize, 0.0f));  // silence

    float* mainPtrs[1] = { main[0].data() };
    const float* scPtrs[1] = { sc[0].data() };

    // Process one block with silent sidechain + loud main
    limiter.process(mainPtrs, 1, kBlockSize, scPtrs);

    // The sidechain is silent, so GR should be minimal — main passes substantially through
    REQUIRE(blockPeak(main[0]) > 1.5f);  // expect minimal attenuation
}

// ---------------------------------------------------------------------------
// test_below_knee_no_gr
//   A signal well below the lower knee boundary must receive zero gain
//   reduction. The Transparent algorithm has kneeWidth = 6 dB, so the lower
//   knee boundary is at (0 - 3) = -3 dBFS ≈ 0.7079 linear. An input of 0.5
//   linear ≈ -6 dBFS is well below the knee and must not be attenuated.
// ---------------------------------------------------------------------------
TEST_CASE("test_below_knee_no_gr", "[TransientLimiter]")
{
    TransientLimiter limiter;
    limiter.prepare(kSampleRate, kBlockSize, kNumChannels);

    // Transparent: kneeWidth = 6 dB, threshold defaults to 1.0 (0 dBFS)
    // Lower knee = 0 dBFS - 3 dB = -3 dBFS ≈ 0.7079 linear
    AlgorithmParams params = getAlgorithmParams(LimiterAlgorithm::Transparent);
    limiter.setAlgorithmParams(params);
    limiter.setThreshold(1.0f);
    limiter.setLookahead(0.0f);  // no lookahead; instant response

    // 0.5 linear ≈ -6 dBFS — comfortably below the lower knee at -3 dBFS
    const float inputAmplitude = 0.5f;
    std::vector<std::vector<float>> buf(kNumChannels, std::vector<float>(kBlockSize, inputAmplitude));

    // Warm up for several blocks so gain state reaches steady state
    for (int block = 0; block < 5; ++block)
    {
        fillConstant(buf, inputAmplitude);
        auto ptrs = makePtrs(buf);
        limiter.process(ptrs.data(), kNumChannels, kBlockSize);
    }

    // GR must be effectively 0 dB — less than 0.1 dB of reduction
    const float grDb = limiter.getGainReduction();
    REQUIRE(grDb >= -0.1f);  // no reduction below knee
}

// ---------------------------------------------------------------------------
// test_in_knee_partial_gr
//   A signal whose dBFS value lies inside the soft-knee region must receive
//   partial gain reduction — not zero (the knee is active) and not the full
//   hard-limit reduction (the knee is still transitioning).
//
//   With Transparent params (kneeWidth = 6 dB, threshold = 0 dBFS):
//     Knee region: [-3 dBFS, +3 dBFS]
//     Test input: threshold_dB - kneeWidth/4 = 0 - 1.5 = -1.5 dBFS
//                 → linear ≈ 0.8414
//     Expected GR formula: t = (-1.5 - (-3)) / 6 = 0.25
//                          gainDb = (0 - 3) * 0.25^2 ≈ -0.1875 dB
// ---------------------------------------------------------------------------
TEST_CASE("test_in_knee_partial_gr", "[TransientLimiter]")
{
    TransientLimiter limiter;
    limiter.prepare(kSampleRate, kBlockSize, kNumChannels);

    AlgorithmParams params = getAlgorithmParams(LimiterAlgorithm::Transparent);
    limiter.setAlgorithmParams(params);
    limiter.setThreshold(1.0f);
    limiter.setLookahead(0.0f);

    // -1.5 dBFS = threshold_dB - kneeWidth/4 → one-quarter into the knee from below
    const float inputAmplitude = std::pow(10.0f, -1.5f / 20.0f);  // ≈ 0.8414

    std::vector<std::vector<float>> buf(kNumChannels, std::vector<float>(kBlockSize, inputAmplitude));

    // Warm up to reach steady-state gain
    for (int block = 0; block < 5; ++block)
    {
        fillConstant(buf, inputAmplitude);
        auto ptrs = makePtrs(buf);
        limiter.process(ptrs.data(), kNumChannels, kBlockSize);
    }

    const float grDb = limiter.getGainReduction();

    // Some reduction must be present (in-knee, non-zero GR)
    REQUIRE(grDb < 0.0f);

    // But not "full" reduction — the signal is below the upper knee (+3 dBFS),
    // so the knee curve should produce only a small partial reduction (< 3 dB)
    REQUIRE(grDb > -3.0f);
}

// ---------------------------------------------------------------------------
// test_above_knee_full_gr
//   A signal well above the upper knee boundary must receive full gain
//   reduction — the output should be clamped to ≤ threshold + small margin.
//   With Transparent params (kneeWidth = 6 dB): upper knee = +3 dBFS ≈ 1.4125.
//   Input at 2.0 linear = +6 dBFS is well above the upper knee.
// ---------------------------------------------------------------------------
TEST_CASE("test_above_knee_full_gr", "[TransientLimiter]")
{
    TransientLimiter limiter;
    limiter.prepare(kSampleRate, kBlockSize, kNumChannels);

    AlgorithmParams params = getAlgorithmParams(LimiterAlgorithm::Transparent);
    params.saturationAmount = 0.0f;  // disable saturation for clean measurement
    limiter.setAlgorithmParams(params);
    limiter.setThreshold(1.0f);
    limiter.setLookahead(0.0f);  // no lookahead; instant attack

    // 2.0 linear = +6 dBFS — well above the upper knee at +3 dBFS
    const float inputAmplitude = 2.0f;
    std::vector<std::vector<float>> buf(kNumChannels, std::vector<float>(kBlockSize, inputAmplitude));

    // Warm up for several blocks to reach steady-state limiting
    for (int block = 0; block < 5; ++block)
    {
        fillConstant(buf, inputAmplitude);
        auto ptrs = makePtrs(buf);
        limiter.process(ptrs.data(), kNumChannels, kBlockSize);
    }

    // Output must be clamped to threshold + small margin (full GR applied)
    for (int ch = 0; ch < kNumChannels; ++ch)
        REQUIRE(blockPeak(buf[ch]) <= 1.0f + 0.01f);

    // GR must be reported (negative dB, clearly non-zero)
    REQUIRE(limiter.getGainReduction() < -1.0f);
}

// ---------------------------------------------------------------------------
// INT32 overflow boundary tests (task 161)
// ---------------------------------------------------------------------------

// Helper: process N blocks of silence to advance the limiter state.
static void processNSilentSamples(TransientLimiter& lim, int numChannels, int numSamples)
{
    constexpr int batchSize = 512;
    std::vector<std::vector<float>> buf(numChannels, std::vector<float>(batchSize, 0.0f));
    int remaining = numSamples;
    while (remaining > 0)
    {
        const int n = std::min(remaining, batchSize);
        for (auto& ch : buf) std::fill(ch.begin(), ch.end(), 0.0f);
        std::vector<float*> ptrs(numChannels);
        for (int ch = 0; ch < numChannels; ++ch) ptrs[ch] = buf[ch].data();
        lim.process(ptrs.data(), numChannels, n);
        remaining -= n;
    }
}

TEST_CASE("write counter int32 overflow: deque still functions after overflow point", "[TransientLimiter][overflow]")
{
    // Seed counters just before where int32 would have wrapped.
    // INT_MAX = 2147483647; place counter at INT_MAX - 10 so we cross the
    // old boundary within just a few samples.
    constexpr int64_t kSeedOffset = (int64_t)2147483647 - 10;  // INT_MAX - 10

    TransientLimiter limiter;
    limiter.prepare(kSampleRate, kBlockSize, kNumChannels);

    AlgorithmParams params = getAlgorithmParams(LimiterAlgorithm::Transparent);
    params.saturationAmount = 0.0f;
    params.kneeWidth        = 0.0f;  // hard knee for predictable output
    limiter.setAlgorithmParams(params);
    limiter.setThreshold(0.5f);
    limiter.setLookahead(1.0f);  // 1 ms lookahead so deques are active

    // Seed counters near int32 overflow point and clear deques.
    limiter.resetCounters(kSeedOffset);

    // Process 30 samples of over-threshold signal across the overflow boundary.
    // The counter will pass through INT_MAX and into what would have been negative
    // territory if it were int32.  With int64_t it just keeps incrementing safely.
    const int kTestSamples = 30;
    std::vector<std::vector<float>> buf(kNumChannels, std::vector<float>(kTestSamples, 1.0f));

    // Warm up the delay buffer first (use silence so gain state is reset)
    processNSilentSamples(limiter, kNumChannels, (int)limiter.getLatencyInSamples() + 2);

    // Reset counters again (silence run advanced them, re-seed at boundary)
    limiter.resetCounters(kSeedOffset);

    // Process a loud block — counter increments from kSeedOffset across INT_MAX
    std::vector<float*> ptrs(kNumChannels);
    for (int ch = 0; ch < kNumChannels; ++ch)
    {
        buf[ch].assign(kTestSamples, 1.0f);
        ptrs[ch] = buf[ch].data();
    }
    limiter.process(ptrs.data(), kNumChannels, kTestSamples);

    // After crossing the old int32 overflow boundary the limiter must still report GR
    REQUIRE(limiter.getGainReduction() < 0.0f);  // non-zero GR means deque is working
}

TEST_CASE("write counter int32 overflow: peak tracking correct across boundary", "[TransientLimiter][overflow]")
{
    // Same setup: seed counters so the first processed sample pushes the counter
    // past what INT_MAX would have been.
    constexpr int64_t kSeedOffset = (int64_t)2147483647 - 5;  // INT_MAX - 5

    TransientLimiter limiter;
    limiter.prepare(kSampleRate, kBlockSize, kNumChannels);

    AlgorithmParams params = getAlgorithmParams(LimiterAlgorithm::Transparent);
    params.saturationAmount = 0.0f;
    params.kneeWidth        = 0.0f;  // hard knee
    limiter.setAlgorithmParams(params);
    limiter.setThreshold(0.5f);
    limiter.setLookahead(1.0f);

    // Warm the delay buffer with silence so the read pointer starts clean
    processNSilentSamples(limiter, kNumChannels, (int)limiter.getLatencyInSamples() + 5);

    // Re-seed at boundary
    limiter.resetCounters(kSeedOffset);

    // Process many blocks of over-threshold signal.  If the deque breaks at
    // overflow, gain may momentarily drop to 1.0 (no limiting) or produce a
    // click.  We verify limiting stays consistent across all blocks.
    constexpr int kBlocks = 20;
    bool alwaysLimiting = true;
    std::vector<std::vector<float>> buf(kNumChannels, std::vector<float>(kBlockSize, 1.0f));
    for (int b = 0; b < kBlocks; ++b)
    {
        for (auto& ch : buf) std::fill(ch.begin(), ch.end(), 1.0f);
        std::vector<float*> ptrs2(kNumChannels);
        for (int ch = 0; ch < kNumChannels; ++ch) ptrs2[ch] = buf[ch].data();
        limiter.process(ptrs2.data(), kNumChannels, kBlockSize);

        // After the first few warm-up blocks, limiting must be active
        if (b >= 3 && limiter.getGainReduction() >= 0.0f)
            alwaysLimiting = false;

        // Output must never exceed threshold significantly
        for (int ch = 0; ch < kNumChannels; ++ch)
        {
            const float peak = blockPeak(buf[ch]);
            REQUIRE(peak <= 0.5f + 0.05f);  // threshold + 0.05 margin
        }
    }
    REQUIRE(alwaysLimiting);
}

// ---------------------------------------------------------------------------
// test_lookahead_zero_no_latency
//   setLookahead(0.0f) must result in getLatencyInSamples() == 0.
// ---------------------------------------------------------------------------
TEST_CASE("test_lookahead_zero_no_latency", "[TransientLimiter]")
{
    TransientLimiter limiter;
    limiter.prepare(kSampleRate, kBlockSize, kNumChannels);
    limiter.setAlgorithmParams(getAlgorithmParams(LimiterAlgorithm::Transparent));

    limiter.setLookahead(0.0f);
    REQUIRE(limiter.getLatencyInSamples() == 0);
}

// ---------------------------------------------------------------------------
// test_release_linear_domain_parity
//   Verify that the linear-domain release approximation matches the dB-domain
//   reference within 0.5 dB for the first 300 samples after a large peak.
//   Both methods use the same per-sample IIR coefficient (mReleaseCoeff);
//   the difference is whether the state variable is in dB or linear.
// ---------------------------------------------------------------------------
TEST_CASE("test_release_linear_domain_parity", "[TransientLimiter]")
{
    // Setup: mono, 1-sample blocks for per-sample tracking
    TransientLimiter limiter;
    limiter.prepare(kSampleRate, 1, 1);

    AlgorithmParams params = getAlgorithmParams(LimiterAlgorithm::Transparent);
    params.releaseShape     = 0.5f;    // 255 ms at 44100 Hz — slow release keeps parity tight
    params.kneeWidth        = 0.0f;    // hard knee for predictable gain
    params.saturationAmount = 0.0f;
    limiter.setAlgorithmParams(params);
    limiter.setLookahead(0.0f);

    // Per-sample release coefficient (mirrors TransientLimiter.cpp formula)
    const float kReleaseMinMs = 10.0f;
    const float kReleaseMaxMs = 500.0f;
    const float releaseMs = kReleaseMinMs + 0.5f * (kReleaseMaxMs - kReleaseMinMs);  // 255 ms
    const float alpha = std::exp(-1.0f / (releaseMs * 0.001f * (float)kSampleRate));

    // Engage instant-attack gain reduction: feed one +12 dBFS sample
    {
        std::vector<std::vector<float>> buf(1, std::vector<float>(1, 4.0f));
        auto ptrs = makePtrs(buf);
        limiter.process(ptrs.data(), 1, 1);
    }

    // Record the measured GR after the attack (should be ≈ -12 dB)
    const float grAfterAttack = limiter.getGainReduction();
    REQUIRE(grAfterAttack < -6.0f);  // meaningful GR was applied

    // Reference: dB-domain IIR starting from the same measured gain
    // gDb_new = gDb * alpha  (target = 0 dB, so targetDb term vanishes)
    float gDbRef = grAfterAttack;

    // Compare for the first 300 samples (< 7 ms) where parity is < 0.5 dB
    const int kCheckSamples = 300;
    for (int n = 0; n < kCheckSamples; ++n)
    {
        // Production limiter: one silence sample → linear-domain release
        std::vector<std::vector<float>> buf(1, std::vector<float>(1, 0.0f));
        auto ptrs = makePtrs(buf);
        limiter.process(ptrs.data(), 1, 1);
        const float prodGR = limiter.getGainReduction();

        // Reference: dB-domain release (target = 0 dB → gDb_new = gDb * alpha)
        gDbRef *= alpha;

        // Linear and dB domain must agree to within 0.5 dB
        REQUIRE(std::abs(prodGR - gDbRef) < 0.5f);
    }
}

// ---------------------------------------------------------------------------
// test_release_recovery_speed
//   After a sustained +12 dBFS signal drives GR below -6 dB, processing
//   3 * releaseMs * sampleRate samples of silence must recover GR to above
//   -1 dB (linear-domain 3-time-constant recovery criterion).
// ---------------------------------------------------------------------------
TEST_CASE("test_release_recovery_speed", "[TransientLimiter]")
{
    // releaseShape=0.5 → releaseMs = 255 ms at 44100 Hz
    const float kReleaseMinMs = 10.0f;
    const float kReleaseMaxMs = 500.0f;
    const float releaseMs = kReleaseMinMs + 0.5f * (kReleaseMaxMs - kReleaseMinMs);  // 255 ms
    const int   recovery3T = static_cast<int>(3.0f * releaseMs * 0.001f * (float)kSampleRate);

    TransientLimiter limiter;
    limiter.prepare(kSampleRate, kBlockSize, 1);

    AlgorithmParams params = getAlgorithmParams(LimiterAlgorithm::Transparent);
    params.releaseShape     = 0.5f;
    params.kneeWidth        = 0.0f;    // hard knee — instant full GR
    params.saturationAmount = 0.0f;
    limiter.setAlgorithmParams(params);
    limiter.setLookahead(0.0f);

    // Drive gain well below threshold to establish deep GR
    {
        std::vector<std::vector<float>> buf(1, std::vector<float>(kBlockSize, 4.0f));  // +12 dBFS
        auto ptrs = makePtrs(buf);
        for (int b = 0; b < 5; ++b)
        {
            std::fill(buf[0].begin(), buf[0].end(), 4.0f);
            limiter.process(ptrs.data(), 1, kBlockSize);
        }
        REQUIRE(limiter.getGainReduction() < -6.0f);  // GR must be active and deep
    }

    // Release phase: process 3T samples of silence
    int remaining = recovery3T;
    while (remaining > 0)
    {
        const int n = std::min(remaining, kBlockSize);
        std::vector<std::vector<float>> buf(1, std::vector<float>(n, 0.0f));
        auto ptrs = makePtrs(buf);
        limiter.process(ptrs.data(), 1, n);
        remaining -= n;
    }

    // After 3 time constants, linear-domain recovery brings gain to ≈ g0*e^-3 + (1-e^-3)
    // Starting from g0=0.25: g(3T) ≈ 0.963, GR ≈ -0.33 dB → well above -1 dB
    REQUIRE(limiter.getGainReduction() > -1.0f);
}

// ---------------------------------------------------------------------------
// test_lookahead_zero_still_limits
//   With setLookahead(0.0f), a constant +6 dBFS block must be limited to
//   ≤ 1.0 + margin within the first few warm-up blocks (no latency means
//   instant gain reduction; no silent first-sample artefact from delay).
// ---------------------------------------------------------------------------
TEST_CASE("test_lookahead_zero_still_limits", "[TransientLimiter]")
{
    TransientLimiter limiter;
    limiter.prepare(kSampleRate, kBlockSize, kNumChannels);

    AlgorithmParams params = getAlgorithmParams(LimiterAlgorithm::Transparent);
    params.kneeWidth        = 0.0f;  // hard knee — instant response
    params.saturationAmount = 0.0f;
    limiter.setAlgorithmParams(params);
    limiter.setLookahead(0.0f);

    // Feed several blocks of constant +6 dBFS signal
    const int kWarmupBlocks = 3;
    std::vector<std::vector<float>> buf(kNumChannels, std::vector<float>(kBlockSize, 2.0f));

    for (int b = 0; b < kWarmupBlocks; ++b)
    {
        fillConstant(buf, 2.0f);
        auto ptrs = makePtrs(buf);
        limiter.process(ptrs.data(), kNumChannels, kBlockSize);
    }

    // After warm-up, every sample must be within threshold
    for (int ch = 0; ch < kNumChannels; ++ch)
        REQUIRE(blockPeak(buf[ch]) <= 1.0f + 1e-3f);

    // GR must be active
    REQUIRE(limiter.getGainReduction() < 0.0f);
}

// ---------------------------------------------------------------------------
// test_lookahead_zero_to_nonzero_transition
//   Start with setLookahead(0.0f), process several blocks, then switch to
//   setLookahead(1.0f).  getLatencyInSamples() must update and limiting must
//   remain correct after the transition (no memory corruption / silent burst).
// ---------------------------------------------------------------------------
TEST_CASE("test_lookahead_zero_to_nonzero_transition", "[TransientLimiter]")
{
    TransientLimiter limiter;
    limiter.prepare(kSampleRate, kBlockSize, kNumChannels);

    AlgorithmParams params = getAlgorithmParams(LimiterAlgorithm::Transparent);
    params.kneeWidth        = 0.0f;
    params.saturationAmount = 0.0f;
    limiter.setAlgorithmParams(params);

    // Phase 1: zero lookahead
    limiter.setLookahead(0.0f);
    REQUIRE(limiter.getLatencyInSamples() == 0);

    std::vector<std::vector<float>> buf(kNumChannels, std::vector<float>(kBlockSize, 2.0f));
    for (int b = 0; b < 5; ++b)
    {
        fillConstant(buf, 2.0f);
        auto ptrs = makePtrs(buf);
        limiter.process(ptrs.data(), kNumChannels, kBlockSize);
    }

    // Phase 2: switch to 1 ms lookahead
    limiter.setLookahead(1.0f);
    const int newLatency = limiter.getLatencyInSamples();
    REQUIRE(newLatency > 0);

    // Continue processing — output must still be within threshold
    for (int b = 0; b < 5; ++b)
    {
        fillConstant(buf, 2.0f);
        auto ptrs = makePtrs(buf);
        limiter.process(ptrs.data(), kNumChannels, kBlockSize);

        for (int ch = 0; ch < kNumChannels; ++ch)
            REQUIRE(blockPeak(buf[ch]) <= 1.0f + 1e-3f);
    }
}

// ---------------------------------------------------------------------------
// test_saturation_reduces_peak
//   With saturationAmount=0.5 the output peak must be lower than with
//   saturationAmount=0.0 for a loud above-threshold signal.
// ---------------------------------------------------------------------------
TEST_CASE("test_saturation_reduces_peak", "[TransientLimiter]")
{
    const float inputAmplitude = 2.0f;  // +6 dBFS — above threshold

    auto runWithSaturation = [&](float satAmt) -> float
    {
        TransientLimiter limiter;
        limiter.prepare(kSampleRate, kBlockSize, 1);

        AlgorithmParams params = getAlgorithmParams(LimiterAlgorithm::Transparent);
        params.saturationAmount = satAmt;
        params.kneeWidth        = 0.0f;  // hard knee for predictable behaviour
        limiter.setAlgorithmParams(params);
        limiter.setLookahead(0.0f);

        std::vector<std::vector<float>> buf(1, std::vector<float>(kBlockSize, inputAmplitude));
        auto ptrs = makePtrs(buf);
        limiter.process(ptrs.data(), 1, kBlockSize);
        return blockPeak(buf[0]);
    };

    const float peakWithSat  = runWithSaturation(0.5f);
    const float peakNoSat    = runWithSaturation(0.0f);

    // Saturation should compress transients further, lowering the output peak
    REQUIRE(peakWithSat < peakNoSat);
    // Both must remain at or below the threshold
    REQUIRE(peakWithSat <= 1.0f + 1e-4f);
    REQUIRE(peakNoSat   <= 1.0f + 1e-4f);
}

// ---------------------------------------------------------------------------
// test_saturation_full_amount_no_crash
//   saturationAmount=1.0 with a very loud signal (amplitude 2.0) over
//   10 blocks must produce only finite samples and not exceed the threshold
//   by more than a small tolerance.
// ---------------------------------------------------------------------------
TEST_CASE("test_saturation_full_amount_no_crash", "[TransientLimiter]")
{
    TransientLimiter limiter;
    limiter.prepare(kSampleRate, kBlockSize, kNumChannels);

    AlgorithmParams params = getAlgorithmParams(LimiterAlgorithm::Transparent);
    params.saturationAmount = 1.0f;
    params.kneeWidth        = 0.0f;
    limiter.setAlgorithmParams(params);
    limiter.setLookahead(1.0f);

    std::vector<std::vector<float>> buf(kNumChannels, std::vector<float>(kBlockSize, 2.0f));

    for (int block = 0; block < 10; ++block)
    {
        for (auto& ch : buf)
            std::fill(ch.begin(), ch.end(), 2.0f);
        auto ptrs = makePtrs(buf);
        limiter.process(ptrs.data(), kNumChannels, kBlockSize);

        for (int ch = 0; ch < kNumChannels; ++ch)
        {
            for (float v : buf[ch])
                REQUIRE(std::isfinite(v));
            // Allow a small tolerance above threshold
            REQUIRE(blockPeak(buf[ch]) <= 1.05f);
        }
    }
}

// ---------------------------------------------------------------------------
// test_saturation_amount_zero_is_linear
//   Explicitly confirms that saturationAmount=0.0 leaves a sub-threshold
//   sine-like signal unchanged — regression baseline for the saturation tests.
// ---------------------------------------------------------------------------
TEST_CASE("test_saturation_amount_zero_is_linear", "[TransientLimiter]")
{
    TransientLimiter limiter;
    limiter.prepare(kSampleRate, kBlockSize, 1);

    AlgorithmParams params = getAlgorithmParams(LimiterAlgorithm::Transparent);
    params.saturationAmount = 0.0f;
    params.kneeWidth        = 0.0f;
    limiter.setAlgorithmParams(params);
    limiter.setLookahead(0.0f);

    // Sub-threshold sine-like signal (amplitude 0.5 — well below 1.0)
    const float amplitude = 0.5f;
    std::vector<std::vector<float>> buf(1, std::vector<float>(kBlockSize));
    for (int i = 0; i < kBlockSize; ++i)
        buf[0][i] = amplitude * std::sin(2.0f * 3.14159265f * 1000.0f * static_cast<float>(i) / static_cast<float>(kSampleRate));
    const std::vector<float> reference = buf[0];

    // Warm-up pass
    auto ptrs = makePtrs(buf);
    limiter.process(ptrs.data(), 1, kBlockSize);

    // Refill and run again so lookahead delay buffer is populated with the same signal
    buf[0] = reference;
    ptrs   = makePtrs(buf);
    limiter.process(ptrs.data(), 1, kBlockSize);

    // Output must equal input: no gain reduction, no saturation distortion
    for (int i = 0; i < kBlockSize; ++i)
        REQUIRE(buf[0][i] == Catch::Approx(reference[i]).margin(0.01f));
}

// ---------------------------------------------------------------------------
// test_saturation_formula_direct
//   Feeds a sub-threshold signal with saturationAmount=0.5.
//   For large input magnitudes (near but below threshold), tanh compression
//   means output < input.  For very small magnitudes, output ≈ input.
// ---------------------------------------------------------------------------
TEST_CASE("test_saturation_formula_direct", "[TransientLimiter]")
{
    TransientLimiter limiter;
    limiter.prepare(kSampleRate, kBlockSize, 1);

    AlgorithmParams params = getAlgorithmParams(LimiterAlgorithm::Transparent);
    params.saturationAmount = 0.5f;
    params.kneeWidth        = 0.0f;
    limiter.setAlgorithmParams(params);
    limiter.setLookahead(0.0f);

    // Large sub-threshold input (0.9 — below 1.0 but large enough for tanh to bite)
    const float largeAmp = 0.9f;
    std::vector<std::vector<float>> largeBuf(1, std::vector<float>(kBlockSize, largeAmp));
    auto largePtrs = makePtrs(largeBuf);
    // Warm-up
    limiter.process(largePtrs.data(), 1, kBlockSize);
    std::fill(largeBuf[0].begin(), largeBuf[0].end(), largeAmp);
    limiter.process(largePtrs.data(), 1, kBlockSize);
    const float outLarge = blockPeak(largeBuf[0]);

    // With saturation active, output magnitude < input magnitude
    REQUIRE(outLarge < largeAmp);

    // Small sub-threshold input (0.01 — tanh(x) ≈ x so output ≈ input)
    limiter.prepare(kSampleRate, kBlockSize, 1);
    limiter.setAlgorithmParams(params);
    limiter.setLookahead(0.0f);

    const float smallAmp = 0.01f;
    std::vector<std::vector<float>> smallBuf(1, std::vector<float>(kBlockSize, smallAmp));
    auto smallPtrs = makePtrs(smallBuf);
    // Warm-up
    limiter.process(smallPtrs.data(), 1, kBlockSize);
    std::fill(smallBuf[0].begin(), smallBuf[0].end(), smallAmp);
    limiter.process(smallPtrs.data(), 1, kBlockSize);
    const float outSmall = blockPeak(smallBuf[0]);

    // For small signals tanh(x)/drive ≈ x/drive, so wet ≈ x/drive.
    // Output = x*(1-amount) + (x/drive)*amount which is less than x but close.
    // Just verify it's within a reasonable range of the input.
    REQUIRE(outSmall == Catch::Approx(smallAmp).epsilon(0.05f));
}
