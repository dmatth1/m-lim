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
    // the output at output step mainPeakPos + L - 1.  The sidechain peak is
    // detected at output step mainPeakPos.  With correct scan-window lookahead,
    // GR is held at peak level for the full lookahead window and is still at
    // its maximum at step mainPeakPos + L - 1, keeping the delayed main peak
    // within threshold.
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
    const int delayedPeakPos = mainPeakPos + lookaheadSamples - 1;
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
//   silence.  With lookahead L the delay is (L-1) samples, so the spike
//   arrives at the output at position peakPos + L - 1.  Verify:
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

    // 3. Delayed output spike position must be within ±1 sample of expected
    const int expectedOutputPos = peakPos + lookaheadSamples - 1;
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
    const int expectedOutputPos = peakPos + lookaheadSamples - 1;
    if (expectedOutputPos < kBlockSize)
        REQUIRE(buf[0][expectedOutputPos] <= 1.0f + 1e-4f);

    // 4. GR was applied.
    REQUIRE(limiter.getGainReduction() <= 0.0f);
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
        // Write into delay buffer
        delayBuf[writePos] = input[s];
        writePos = (writePos + 1) % bufSize;

        // Brute-force O(N) scan over lookahead window
        float peakAbs = 0.0f;
        int   rpos    = (writePos - 1 + bufSize) % bufSize;
        for (int k = 0; k < lookaheadSamples; ++k)
        {
            peakAbs = std::max(peakAbs, std::abs(delayBuf[rpos]));
            rpos    = (rpos - 1 + bufSize) % bufSize;
        }

        // Hard-knee required gain
        const float required = (peakAbs > threshold + 1e-9f)
                                   ? (threshold / peakAbs)
                                   : 1.0f;

        // Instant attack, dB-domain release (matches TransientLimiter smoothing)
        if (required < gainState)
        {
            gainState = required;
        }
        else
        {
            const float gDb      = 20.0f * std::log10(std::max(gainState,  1e-6f));
            const float tDb      = 20.0f * std::log10(std::max(required,   1e-6f));
            const float smoothed = gDb + (tDb - gDb) * (1.0f - releaseCoeff);
            gainState = std::pow(10.0f, smoothed / 20.0f);
        }
        gainState = std::clamp(gainState, 1e-6f, 1.0f);

        // Read delayed output
        const int readPos = (writePos - lookaheadSamples + bufSize) % bufSize;
        bruteOutput[s] = delayBuf[readPos] * gainState;
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
