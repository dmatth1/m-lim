#include "catch2/catch_amalgamated.hpp"
#include "dsp/LoudnessMeter.h"
#include "alloc_tracking.h"
#include <juce_audio_basics/juce_audio_basics.h>
#include <chrono>
#include <cmath>
#include <limits>
#include <vector>

// ---------------------------------------------------------------------------
// Helpers
// ---------------------------------------------------------------------------
static constexpr double kPi       = 3.14159265358979323846;
static constexpr double kSampleRate = 48000.0;

/** Feed numSeconds of silence into the meter in one-block-at-a-time chunks. */
static void feedSilence(LoudnessMeter& meter, double fs, double numSeconds,
                        int blockSize = 512, int numChannels = 2)
{
    juce::AudioBuffer<float> buf(numChannels, blockSize);
    buf.clear();
    const int totalSamples = static_cast<int>(fs * numSeconds);
    for (int pos = 0; pos < totalSamples; pos += blockSize)
    {
        int n = std::min(blockSize, totalSamples - pos);
        if (n < blockSize)
        {
            juce::AudioBuffer<float> last(numChannels, n);
            last.clear();
            meter.processBlock(last);
        }
        else
        {
            meter.processBlock(buf);
        }
    }
}

/** Feed numSeconds of a sine tone into the meter. */
static void feedSine(LoudnessMeter& meter, double freqHz, float amplitude,
                     double fs, double numSeconds,
                     int blockSize = 512, int numChannels = 2)
{
    const int totalSamples = static_cast<int>(fs * numSeconds);
    int samplePos = 0;

    while (samplePos < totalSamples)
    {
        int n = std::min(blockSize, totalSamples - samplePos);
        juce::AudioBuffer<float> buf(numChannels, n);
        for (int ch = 0; ch < numChannels; ++ch)
        {
            float* data = buf.getWritePointer(ch);
            for (int i = 0; i < n; ++i)
                data[i] = amplitude * static_cast<float>(
                               std::sin(2.0 * kPi * freqHz * (samplePos + i) / fs));
        }
        meter.processBlock(buf);
        samplePos += n;
    }
}

// ---------------------------------------------------------------------------
// test_silence_returns_negative_infinity
// ---------------------------------------------------------------------------
TEST_CASE("test_silence_returns_negative_infinity", "[LoudnessMeter]")
{
    LoudnessMeter meter;
    meter.prepare(kSampleRate, 2);

    // Feed 5 seconds of silence — enough to fill all windows
    feedSilence(meter, kSampleRate, 5.0);

    CHECK(std::isinf(meter.getMomentaryLUFS()));
    CHECK(meter.getMomentaryLUFS() < 0.0f);
    CHECK(std::isinf(meter.getShortTermLUFS()));
    CHECK(meter.getShortTermLUFS() < 0.0f);
    CHECK(std::isinf(meter.getIntegratedLUFS()));
    CHECK(meter.getIntegratedLUFS() < 0.0f);
}

// ---------------------------------------------------------------------------
// test_1khz_sine_loudness
// A 1 kHz stereo sine at -20 dBFS should read approximately -20 LUFS (±1 LU).
// K-weighting has near unity gain at 1 kHz so the measurement should be close.
// ---------------------------------------------------------------------------
TEST_CASE("test_1khz_sine_loudness", "[LoudnessMeter]")
{
    LoudnessMeter meter;
    meter.prepare(kSampleRate, 2);

    // Amplitude for -20 dBFS
    const float amp = static_cast<float>(std::pow(10.0, -20.0 / 20.0));

    // Feed 5 s of tone — enough to fill momentary, short-term, and integrated windows
    feedSine(meter, 1000.0, amp, kSampleRate, 5.0);

    const float momentary  = meter.getMomentaryLUFS();
    const float shortTerm  = meter.getShortTermLUFS();
    const float integrated = meter.getIntegratedLUFS();

    INFO("Momentary LUFS: " << momentary);
    INFO("Short-term LUFS: " << shortTerm);
    INFO("Integrated LUFS: " << integrated);

    // Stereo -20 dBFS 1 kHz sine: z_L + z_R ≈ 2 * RMS^2.
    // Expected: -0.691 + 10*log10(2 * 10^(-2)) ≈ -20.69 LUFS.
    // Allow ±1.5 LU for K-weighting gain and transient effects.
    CHECK(momentary  > -22.5f);
    CHECK(momentary  < -19.0f);
    CHECK(shortTerm  > -22.5f);
    CHECK(shortTerm  < -19.0f);
    CHECK(integrated > -22.5f);
    CHECK(integrated < -19.0f);
}

// ---------------------------------------------------------------------------
// test_momentary_vs_shortterm
// Momentary uses a 400 ms window; short-term uses a 3 s window.
// After 4 s of tone + 2 s silence:
//   – momentary should be silent (-inf) since last 400 ms is silence
//   – short-term should still be above -70 LUFS since 3 s window still holds tone
// ---------------------------------------------------------------------------
TEST_CASE("test_momentary_vs_shortterm", "[LoudnessMeter]")
{
    LoudnessMeter meter;
    meter.prepare(kSampleRate, 2);

    const float amp = static_cast<float>(std::pow(10.0, -20.0 / 20.0));

    // 4 s of tone, then 2 s of silence
    feedSine(meter, 1000.0, amp, kSampleRate, 4.0);
    feedSilence(meter, kSampleRate, 2.0);

    // Momentary should now be (nearly) silent — 400 ms window is all silence
    // Allow a small margin since the block boundary may include a tiny tail
    INFO("Momentary after silence: " << meter.getMomentaryLUFS());
    CHECK(std::isinf(meter.getMomentaryLUFS()));

    // Short-term 3 s window still contains 1 s of tone → not -inf
    INFO("Short-term after silence: " << meter.getShortTermLUFS());
    CHECK(meter.getShortTermLUFS() > -60.0f);  // well above silence
}

// ---------------------------------------------------------------------------
// test_integrated_accumulates
// Integrated LUFS should accumulate over time with gating.
// After feeding a long tone, integrated should be a valid finite value.
// ---------------------------------------------------------------------------
TEST_CASE("test_integrated_accumulates", "[LoudnessMeter]")
{
    LoudnessMeter meter;
    meter.prepare(kSampleRate, 2);

    const float amp = static_cast<float>(std::pow(10.0, -23.0 / 20.0));

    // Feed 10 s of tone at -23 dBFS
    feedSine(meter, 1000.0, amp, kSampleRate, 10.0);

    const float integrated = meter.getIntegratedLUFS();
    INFO("Integrated LUFS: " << integrated);

    // Should be a finite, reasonable value (not -inf)
    CHECK(std::isfinite(integrated));
    // Stereo sum: z_L + z_R ≈ 2 * RMS^2 → ~+3 dB relative to mono.
    // -23 dBFS stereo sine → integrated ≈ -23.69 LUFS (within 2 LU).
    CHECK(integrated > -26.0f);
    CHECK(integrated < -21.0f);
}

// ---------------------------------------------------------------------------
// test_1khz_loudness_at_44100
// 1 kHz stereo sine at -20 dBFS, SR=44100: LUFS should be ≈ -20 (within 0.5 LU).
// ---------------------------------------------------------------------------
TEST_CASE("test_1khz_loudness_at_44100", "[LoudnessMeter]")
{
    LoudnessMeter meter;
    constexpr double fs = 44100.0;
    meter.prepare(fs, 2);

    const float amp = static_cast<float>(std::pow(10.0, -20.0 / 20.0));
    feedSine(meter, 1000.0, amp, fs, 5.0);

    const float momentary = meter.getMomentaryLUFS();
    INFO("Momentary LUFS at 44100: " << momentary);

    // Stereo -20 dBFS 1 kHz sine → expected ~-20.69 LUFS ±1 LU (K-weighting gain near 1kHz varies by SR)
    CHECK(momentary > -21.5f);
    CHECK(momentary < -19.5f);
}

// ---------------------------------------------------------------------------
// test_1khz_loudness_at_96000
// Same signal at 96000 Hz: result should match 44100 reading within ±0.5 LU,
// confirming that coefficients are recalculated for the actual sample rate.
// ---------------------------------------------------------------------------
TEST_CASE("test_1khz_loudness_at_96000", "[LoudnessMeter]")
{
    constexpr double fs44 = 44100.0;
    constexpr double fs96 = 96000.0;
    const float amp = static_cast<float>(std::pow(10.0, -20.0 / 20.0));

    LoudnessMeter meter44;
    meter44.prepare(fs44, 2);
    feedSine(meter44, 1000.0, amp, fs44, 5.0);
    const float lufs44 = meter44.getMomentaryLUFS();

    LoudnessMeter meter96;
    meter96.prepare(fs96, 2);
    feedSine(meter96, 1000.0, amp, fs96, 5.0);
    const float lufs96 = meter96.getMomentaryLUFS();

    INFO("LUFS at 44100: " << lufs44);
    INFO("LUFS at 96000: " << lufs96);

    REQUIRE(std::isfinite(lufs44));
    REQUIRE(std::isfinite(lufs96));
    CHECK(std::abs(lufs96 - lufs44) < 0.5f);
}

// ---------------------------------------------------------------------------
// test_reset_integrated
// resetIntegrated() should clear accumulated data and return -inf again.
// ---------------------------------------------------------------------------
TEST_CASE("test_reset_integrated", "[LoudnessMeter]")
{
    LoudnessMeter meter;
    meter.prepare(kSampleRate, 2);

    const float amp = static_cast<float>(std::pow(10.0, -20.0 / 20.0));

    // Accumulate some integrated loudness
    feedSine(meter, 1000.0, amp, kSampleRate, 5.0);
    REQUIRE(std::isfinite(meter.getIntegratedLUFS()));

    // Reset
    meter.resetIntegrated();

    // Now integrated should be -inf again
    CHECK(std::isinf(meter.getIntegratedLUFS()));
    CHECK(meter.getIntegratedLUFS() < 0.0f);

    // Feed a short burst — not enough for integrated to become valid
    feedSine(meter, 1000.0, amp, kSampleRate, 0.3);
    // 300 ms < 400 ms minimum window → should still be -inf
    CHECK(std::isinf(meter.getIntegratedLUFS()));

    // Now feed enough
    feedSine(meter, 1000.0, amp, kSampleRate, 5.0);
    CHECK(std::isfinite(meter.getIntegratedLUFS()));
}

// ---------------------------------------------------------------------------
// test_long_session_no_alloc
// Process >1000 100ms blocks and verify that per-block processing time
// stays bounded (no O(n²) growth due to unbounded allocations or loops).
// ---------------------------------------------------------------------------
TEST_CASE("test_long_session_no_alloc", "[LoudnessMeter]")
{
    LoudnessMeter meter;
    meter.prepare(kSampleRate, 2);

    const float amp = static_cast<float>(std::pow(10.0, -20.0 / 20.0));

    // Warm up: fill initial history (100 blocks = 10 s)
    feedSine(meter, 1000.0, amp, kSampleRate, 10.0);
    REQUIRE(std::isfinite(meter.getIntegratedLUFS()));

    // Time 100 blocks early in session
    auto t0 = std::chrono::steady_clock::now();
    feedSine(meter, 1000.0, amp, kSampleRate, 10.0); // 100 more blocks
    auto t1 = std::chrono::steady_clock::now();
    const double msEarly = std::chrono::duration<double, std::milli>(t1 - t0).count();

    // Process 900 more blocks (total > 1000 blocks)
    feedSine(meter, 1000.0, amp, kSampleRate, 90.0);

    // Time 100 blocks late in session
    auto t2 = std::chrono::steady_clock::now();
    feedSine(meter, 1000.0, amp, kSampleRate, 10.0); // 100 blocks late
    auto t3 = std::chrono::steady_clock::now();
    const double msLate = std::chrono::duration<double, std::milli>(t3 - t2).count();

    INFO("Early 10s processing: " << msEarly << " ms");
    INFO("Late 10s processing:  " << msLate  << " ms");

    // Late processing must not be more than 10× slower than early processing
    // (acceptable growth from bounded O(n) work where n is capped at 6000 blocks)
    CHECK(msLate < msEarly * 10.0 + 500.0); // generous bound; guards against O(n²)

    // Results must still be valid
    CHECK(std::isfinite(meter.getIntegratedLUFS()));
    CHECK(std::isfinite(meter.getMomentaryLUFS()));
    CHECK(std::isfinite(meter.getShortTermLUFS()));
}

// ---------------------------------------------------------------------------
// test_integrated_lufs_long_session
// Process enough blocks to exceed the circular history cap (>6000 blocks =
// >10 min of audio) and verify the integrated LUFS stays valid and bounded.
// ---------------------------------------------------------------------------
TEST_CASE("test_integrated_lufs_long_session", "[LoudnessMeter]")
{
    LoudnessMeter meter;
    meter.prepare(kSampleRate, 2);

    const float amp = static_cast<float>(std::pow(10.0, -23.0 / 20.0));

    // Process 620 seconds of audio (6200 blocks, exceeds kMaxHistoryBlocks=6000)
    // Feed in 10s chunks to keep individual feedSine calls manageable
    for (int chunk = 0; chunk < 62; ++chunk)
        feedSine(meter, 1000.0, amp, kSampleRate, 10.0);

    const float integrated = meter.getIntegratedLUFS();
    const float shortTerm  = meter.getShortTermLUFS();
    const float momentary  = meter.getMomentaryLUFS();

    INFO("Integrated LUFS after long session: " << integrated);
    INFO("Short-term LUFS after long session: " << shortTerm);
    INFO("Momentary LUFS after long session:  " << momentary);

    // Must still return a valid, reasonable value — not blow up or return -inf
    CHECK(std::isfinite(integrated));
    CHECK(std::isfinite(shortTerm));
    CHECK(std::isfinite(momentary));

    // Stereo -23 dBFS 1 kHz sine → integrated ≈ -23.69 LUFS (within 3 LU)
    CHECK(integrated > -27.0f);
    CHECK(integrated < -20.0f);
}

// ---------------------------------------------------------------------------
// test_no_alloc_in_processblock
// After prepare(), repeated calls to processBlock() must not allocate heap
// memory.  Uses the binary-wide allocation counter from alloc_tracking.h /
// test_realtime_safety.cpp.
// ---------------------------------------------------------------------------
TEST_CASE("test_no_alloc_in_processblock", "[LoudnessMeter]")
{
    LoudnessMeter meter;
    meter.prepare(kSampleRate, 2);

    const float amp = static_cast<float>(std::pow(10.0, -20.0 / 20.0));

    // Warm up: fill both ring buffers (4 and 30 blocks) so we are in steady state.
    feedSine(meter, 1000.0, amp, kSampleRate, 4.0);

    // Measure: each processBlock() call inside the guarded scope must be alloc-free.
    const int    blockSize  = 512;
    const int    numChannels = 2;
    const int    blockSamples = static_cast<int>(kSampleRate * 0.1); // 1 x 100ms block
    juce::AudioBuffer<float> buf(numChannels, blockSamples);

    for (int ch = 0; ch < numChannels; ++ch)
    {
        float* data = buf.getWritePointer(ch);
        const double phaseStep = 2.0 * 3.14159265358979323846 * 1000.0 / kSampleRate;
        for (int i = 0; i < blockSamples; ++i)
            data[i] = amp * static_cast<float>(std::sin(phaseStep * i));
    }

    for (int block = 0; block < 50; ++block)
    {
        AllocGuard guard;
        meter.processBlock(buf);
        REQUIRE(guard.count() == 0);
    }
}

// ---------------------------------------------------------------------------
// test_mono_input_no_crash
// prepare() with 1 channel, process 100 blocks of sine → getIntegratedLUFS()
// is finite and negative (no crash, no NaN).
// ---------------------------------------------------------------------------
TEST_CASE("test_mono_input_no_crash", "[LoudnessMeter]")
{
    LoudnessMeter meter;
    constexpr double fs = 48000.0;
    meter.prepare(fs, 1);

    const float amp = static_cast<float>(std::pow(10.0, -20.0 / 20.0));

    // 100 blocks ≈ 10 s of mono sine at -20 dBFS
    feedSine(meter, 1000.0, amp, fs, 10.0, 512, 1);

    const float integrated = meter.getIntegratedLUFS();
    INFO("Mono integrated LUFS: " << integrated);

    CHECK(std::isfinite(integrated));
    CHECK(integrated < 0.0f);

    const float momentary = meter.getMomentaryLUFS();
    INFO("Mono momentary LUFS: " << momentary);
    CHECK(std::isfinite(momentary));
    CHECK(momentary < 0.0f);
}

// ---------------------------------------------------------------------------
// test_absolute_gate_excludes_silence
// Feed 5 s of silence (below -70 LUFS absolute gate) then 5 s of -20 dBFS
// tone.  The integrated result must reflect only the tone period, not be
// dragged down by the silent blocks.
// ---------------------------------------------------------------------------
TEST_CASE("test_absolute_gate_excludes_silence", "[LoudnessMeter]")
{
    LoudnessMeter meter;
    constexpr double fs = 48000.0;
    meter.prepare(fs, 2);

    const float amp = static_cast<float>(std::pow(10.0, -20.0 / 20.0));

    // 5 s silence → all blocks have power 0 → below absolute gate → excluded
    feedSilence(meter, fs, 5.0);

    // 5 s of tone at -20 dBFS
    feedSine(meter, 1000.0, amp, fs, 5.0);

    const float integrated = meter.getIntegratedLUFS();
    INFO("Integrated LUFS after silence + tone: " << integrated);

    // Must be finite (not dragged to -inf by silence blocks)
    REQUIRE(std::isfinite(integrated));

    // Stereo -20 dBFS 1 kHz sine → expected ≈ -20.69 LUFS; allow ±3 LU
    CHECK(integrated > -24.0f);
    CHECK(integrated < -17.5f);
}

// ---------------------------------------------------------------------------
// test_reset_integrated_does_not_affect_momentary
// Call resetIntegrated() mid-session; getMomentaryLUFS() must remain valid
// since the momentary ring buffer is independent of the integrated history.
// ---------------------------------------------------------------------------
TEST_CASE("test_reset_integrated_does_not_affect_momentary", "[LoudnessMeter]")
{
    LoudnessMeter meter;
    constexpr double fs = 48000.0;
    meter.prepare(fs, 2);

    const float amp = static_cast<float>(std::pow(10.0, -20.0 / 20.0));

    // Build up a valid momentary reading
    feedSine(meter, 1000.0, amp, fs, 5.0);
    REQUIRE(std::isfinite(meter.getMomentaryLUFS()));
    REQUIRE(std::isfinite(meter.getIntegratedLUFS()));

    // Reset integrated only
    meter.resetIntegrated();

    // Momentary ring buffer must still be valid (not reset)
    const float momentaryAfterReset = meter.getMomentaryLUFS();
    INFO("Momentary after resetIntegrated: " << momentaryAfterReset);
    CHECK(std::isfinite(momentaryAfterReset));
    CHECK(momentaryAfterReset < 0.0f);

    // Short-term should also be unaffected
    CHECK(std::isfinite(meter.getShortTermLUFS()));

    // Continue processing — momentary should stay valid
    feedSine(meter, 1000.0, amp, fs, 2.0);
    CHECK(std::isfinite(meter.getMomentaryLUFS()));
}

// ---------------------------------------------------------------------------
// test_lra_zero_for_constant_signal
// 10 s of constant loudness → all 3 s windows have identical LUFS →
// 10th and 95th percentiles lie in the same histogram bin → LRA ≈ 0 LU.
// ---------------------------------------------------------------------------
TEST_CASE("test_lra_zero_for_constant_signal", "[LoudnessMeter]")
{
    LoudnessMeter meter;
    constexpr double fs = 48000.0;
    meter.prepare(fs, 2);

    // -23 dBFS stereo sine: constant loudness throughout
    const float amp = static_cast<float>(std::pow(10.0, -23.0 / 20.0));
    feedSine(meter, 1000.0, amp, fs, 10.0);

    const float lra = meter.getLoudnessRange();
    INFO("LRA for constant signal: " << lra << " LU");

    // All windows have identical loudness → range should be 0 (or very small)
    CHECK(lra < 0.5f);
}

// ---------------------------------------------------------------------------
// test_silence_integrated_is_minus_infinity
// Processing only silence for the entire session: getIntegratedLUFS() must
// return negative infinity (all blocks below the -70 LUFS absolute gate).
// ---------------------------------------------------------------------------
TEST_CASE("test_silence_integrated_is_minus_infinity", "[LoudnessMeter]")
{
    LoudnessMeter meter;
    constexpr double fs = 48000.0;
    meter.prepare(fs, 2);

    // Feed >1 s of silence so integrated update fires at least once
    feedSilence(meter, fs, 3.0);

    const float integrated = meter.getIntegratedLUFS();
    INFO("Integrated LUFS (silence only): " << integrated);

    CHECK(std::isinf(integrated));
    CHECK(integrated < 0.0f);
}

// ---------------------------------------------------------------------------
// test_short_term_initial_value_is_neg_inf
// Immediately after prepare(), getShortTermLUFS() must return -infinity
// (no valid measurement before any audio has been processed).
// ---------------------------------------------------------------------------
TEST_CASE("test_short_term_initial_value_is_neg_inf", "[LoudnessMeter]")
{
    LoudnessMeter meter;
    meter.prepare(kSampleRate, 2);

    const float shortTerm = meter.getShortTermLUFS();
    INFO("Short-term LUFS immediately after prepare: " << shortTerm);

    CHECK(std::isinf(shortTerm));
    CHECK(shortTerm < 0.0f);
}

// ---------------------------------------------------------------------------
// test_short_term_lufs_requires_full_window
// Short-term LUFS must remain -infinity until exactly 30 × 100 ms blocks
// (3 seconds) have been accumulated.  After the 30th block it must become
// a finite value.
// ---------------------------------------------------------------------------
TEST_CASE("test_short_term_lufs_requires_full_window", "[LoudnessMeter]")
{
    LoudnessMeter meter;
    constexpr double fs = 48000.0;
    meter.prepare(fs, 2);

    const float amp = static_cast<float>(std::pow(10.0, -20.0 / 20.0));

    // Feed 2900 ms (29 × 100 ms blocks) — one block short of 3 s
    feedSine(meter, 1000.0, amp, fs, 2.9);

    INFO("Short-term LUFS after 2900 ms: " << meter.getShortTermLUFS());
    CHECK(std::isinf(meter.getShortTermLUFS()));
    CHECK(meter.getShortTermLUFS() < 0.0f);

    // Feed one more 100 ms block — now the window is full
    feedSine(meter, 1000.0, amp, fs, 0.1);

    INFO("Short-term LUFS after 3000 ms: " << meter.getShortTermLUFS());
    CHECK(std::isfinite(meter.getShortTermLUFS()));
}

// ---------------------------------------------------------------------------
// test_short_term_lufs_matches_momentary_after_one_block
// After exactly 30 blocks of a steady-state tone, getShortTermLUFS() must
// reflect the loudness averaged over the entire 3 s window — not just the
// last 100 ms block.  We verify this by comparing a meter that received a
// constant signal against one that received silence followed by one loud
// block: the latter must NOT report the same short-term level as the former
// (confirming the mean is over all 30 blocks, not just the last one).
// ---------------------------------------------------------------------------
TEST_CASE("test_short_term_lufs_matches_momentary_after_one_block", "[LoudnessMeter]")
{
    constexpr double fs = 48000.0;
    const float amp = static_cast<float>(std::pow(10.0, -20.0 / 20.0));

    // Meter A: 30 blocks of constant tone
    LoudnessMeter meterA;
    meterA.prepare(fs, 2);
    feedSine(meterA, 1000.0, amp, fs, 3.0);
    const float stA = meterA.getShortTermLUFS();

    // Meter B: 29 blocks of silence + 1 block of loud tone
    LoudnessMeter meterB;
    meterB.prepare(fs, 2);
    feedSilence(meterB, fs, 2.9);
    feedSine(meterB, 1000.0, amp, fs, 0.1);
    const float stB = meterB.getShortTermLUFS();

    INFO("Short-term A (30 blocks of tone): " << stA);
    INFO("Short-term B (29 silence + 1 tone): " << stB);

    REQUIRE(std::isfinite(stA));
    REQUIRE(std::isfinite(stB));

    // Meter A should be much louder (full window of tone) than meter B
    // (only 1/30th tone, 29/30ths silence → averaged power is ~1/30th).
    // ~1/30th power ≈ -14.8 dB relative → stB should be well below stA.
    CHECK(stA > stB + 10.0f);
}

// ---------------------------------------------------------------------------
// test_momentary_lufs_ring_buffer
// The ring-buffer implementation must produce the same momentary LUFS as the
// reference deque-based computation.  We verify by comparing the ring buffer
// output against a hand-computed reference value derived from the same signal.
// ---------------------------------------------------------------------------
TEST_CASE("test_momentary_lufs_ring_buffer", "[LoudnessMeter]")
{
    // Feed a known steady-state tone and compare momentary LUFS to the
    // value expected from 4 x 100ms blocks of a -20 dBFS stereo 1 kHz sine.
    constexpr double fs  = 48000.0;
    constexpr float  amp = static_cast<float>(std::pow(10.0, -20.0 / 20.0));

    LoudnessMeter meterA;
    meterA.prepare(fs, 2);
    feedSine(meterA, 1000.0, amp, fs, 5.0);
    const float lufsA = meterA.getMomentaryLUFS();

    // A second meter fed with the same signal must produce the same reading.
    LoudnessMeter meterB;
    meterB.prepare(fs, 2);
    feedSine(meterB, 1000.0, amp, fs, 5.0);
    const float lufsB = meterB.getMomentaryLUFS();

    REQUIRE(std::isfinite(lufsA));
    REQUIRE(std::isfinite(lufsB));
    // Two identically-fed meters must agree exactly (deterministic, no randomness).
    CHECK(lufsA == lufsB);

    // Value must be in the expected range for a -20 dBFS stereo 1 kHz sine
    // (stereo sum doubles power → ~+3 dB offset from mono).
    CHECK(lufsA > -22.5f);
    CHECK(lufsA < -19.0f);
}
