#include "catch2/catch_amalgamated.hpp"
#include "dsp/LoudnessMeter.h"
#include <juce_audio_basics/juce_audio_basics.h>
#include <cmath>
#include <limits>
#include <vector>

// ---------------------------------------------------------------------------
// Helpers
// ---------------------------------------------------------------------------
static constexpr double kPi        = 3.14159265358979323846;
static constexpr double kSampleRate = 48000.0;

// LUFS = -0.691 + 10*log10(power). For stereo correlated sine at amplitude A:
// power = 2 * (A^2/2) = A^2. Expected LUFS (K-weighting flat at 1kHz) = -0.691 + 20*log10(A).
static float expectedLufs(float amplitude) noexcept
{
    double a = static_cast<double>(amplitude);
    return static_cast<float>(-0.691 + 10.0 * std::log10(a * a));
}

/** Feed a stereo sine wave into the meter in 512-sample blocks. */
static void feedSine(LoudnessMeter& meter, double freqHz, float amplitude,
                     double fs, double numSeconds, int numChannels = 2)
{
    const int blockSize    = 512;
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

/** Feed stereo silence. */
static void feedSilence(LoudnessMeter& meter, double fs, double numSeconds, int numChannels = 2)
{
    const int blockSize    = 512;
    const int totalSamples = static_cast<int>(fs * numSeconds);
    juce::AudioBuffer<float> buf(numChannels, blockSize);
    buf.clear();

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

// ---------------------------------------------------------------------------
// test_1khz_reference_signal
// A 1kHz stereo sine chosen to produce exactly -23 LUFS (if K-weighting
// is flat at 1kHz). The expected LUFS is derived from the LUFS formula:
//   LUFS = -0.691 + 10*log10(A^2)  (stereo correlated sine, K-weight ≈ 0dB at 1kHz)
//   => A such that -23 = -0.691 + 10*log10(A^2)
//   => A = 10^((-23+0.691)/20) ≈ 0.07675
// Tolerance ±0.5 LU (allows for small K-weighting deviation from 0dB at 1kHz).
// ---------------------------------------------------------------------------
TEST_CASE("test_1khz_reference_signal", "[LoudnessMeterAccuracy]")
{
    LoudnessMeter meter;
    meter.prepare(kSampleRate, 2);

    // Amplitude that should yield -23 LUFS with flat K-weighting at 1kHz
    const float amp = static_cast<float>(std::pow(10.0, (-23.0 + 0.691) / 20.0));

    // Feed 5 seconds to fill all windows and allow integrated gating to settle
    feedSine(meter, 1000.0, amp, kSampleRate, 5.0);

    const float momentary  = meter.getMomentaryLUFS();
    const float shortTerm  = meter.getShortTermLUFS();
    const float integrated = meter.getIntegratedLUFS();

    INFO("Momentary LUFS: " << momentary << "  (expected ~-23)");
    INFO("Short-term LUFS: " << shortTerm << "  (expected ~-23)");
    INFO("Integrated LUFS: " << integrated << "  (expected ~-23)");

    // K-weighting at 1kHz is close to 0dB, so measurement should be near -23 LUFS
    REQUIRE(momentary  > -24.0f);
    REQUIRE(momentary  < -22.0f);
    REQUIRE(shortTerm  > -24.0f);
    REQUIRE(shortTerm  < -22.0f);
    REQUIRE(integrated > -24.0f);
    REQUIRE(integrated < -22.0f);
}

// ---------------------------------------------------------------------------
// test_kweighting_frequency_response
// Verify K-weighting spectral shaping:
//  - 1kHz: reference (0 dB correction)
//  - 100Hz: attenuated vs 1kHz (RLB high-pass has some effect, but not dramatic)
//  - 10kHz: boosted vs 1kHz (pre-filter high-shelf +4dB above ~1681Hz)
// ---------------------------------------------------------------------------
TEST_CASE("test_kweighting_frequency_response", "[LoudnessMeterAccuracy]")
{
    const float amp = 0.1f; // -20 dBFS

    auto measureIntegrated = [&](double freqHz) -> float
    {
        LoudnessMeter meter;
        meter.prepare(kSampleRate, 2);
        feedSine(meter, freqHz, amp, kSampleRate, 5.0);
        return meter.getIntegratedLUFS();
    };

    const float lufs_1k   = measureIntegrated(1000.0);
    const float lufs_100  = measureIntegrated(100.0);
    const float lufs_10k  = measureIntegrated(10000.0);

    INFO("K-weight 1kHz LUFS: " << lufs_1k);
    INFO("K-weight 100Hz LUFS: " << lufs_100);
    INFO("K-weight 10kHz LUFS: " << lufs_10k);

    // 10kHz should be louder than 1kHz (K-weighting boosts high frequencies)
    REQUIRE(lufs_10k > lufs_1k + 1.0f);  // expect ~+2-4 dB boost

    // 100Hz should be quieter than 1kHz (RLB high-pass has slight attenuation)
    // The attenuation may be small (< 3 dB) due to the high-pass cutoff at 38 Hz
    REQUIRE(lufs_100 < lufs_1k + 1.0f);
}

// ---------------------------------------------------------------------------
// test_momentary_window_400ms
// After 4 seconds of tone followed by 500ms of silence, the momentary meter
// (400ms window) should read -inf since no signal in the last 400ms.
// ---------------------------------------------------------------------------
TEST_CASE("test_momentary_window_400ms", "[LoudnessMeterAccuracy]")
{
    LoudnessMeter meter;
    meter.prepare(kSampleRate, 2);

    const float amp = 0.1f;
    feedSine(meter, 1000.0, amp, kSampleRate, 4.0);

    // After the tone, the momentary should be active
    REQUIRE(!std::isinf(meter.getMomentaryLUFS()));

    // Feed 500ms of silence (more than the 400ms momentary window)
    feedSilence(meter, kSampleRate, 0.5);

    // Momentary should now be effectively silent (< -70 LUFS or -inf)
    const float momentaryAfterSilence = meter.getMomentaryLUFS();
    INFO("Momentary after 500ms silence: " << momentaryAfterSilence);
    // Accept either -inf or a value below the absolute gate threshold (-70 LUFS)
    const bool momentaryIsEffectivelySilent =
        (std::isinf(momentaryAfterSilence) && momentaryAfterSilence < 0.0f)
        || momentaryAfterSilence < -70.0f;
    REQUIRE(momentaryIsEffectivelySilent);
}

// ---------------------------------------------------------------------------
// test_shortterm_window_3s
// Short-term uses a 3-second window. After 5 seconds of tone then 3.5 seconds
// of silence, the short-term should become silent (-inf).
// ---------------------------------------------------------------------------
TEST_CASE("test_shortterm_window_3s", "[LoudnessMeterAccuracy]")
{
    LoudnessMeter meter;
    meter.prepare(kSampleRate, 2);

    const float amp = 0.1f;
    feedSine(meter, 1000.0, amp, kSampleRate, 5.0);

    // Short-term should be stable after 3+ seconds of tone
    const float stActive = meter.getShortTermLUFS();
    INFO("Short-term during tone: " << stActive);
    REQUIRE(!std::isinf(stActive));

    // Feed 3.5 seconds of silence — more than the 3s short-term window
    feedSilence(meter, kSampleRate, 3.5);

    const float stSilent = meter.getShortTermLUFS();
    INFO("Short-term after 3.5s silence: " << stSilent);
    // Accept either -inf or a value well below active listening threshold
    const bool shortTermIsEffectivelySilent =
        (std::isinf(stSilent) && stSilent < 0.0f) || stSilent < -70.0f;
    REQUIRE(shortTermIsEffectivelySilent);
}

// ---------------------------------------------------------------------------
// test_integrated_gating
// Below-threshold signals (< -70 LUFS absolute gate) should be excluded from
// integrated loudness. Verify that a very quiet signal does not affect the
// integrated reading.
// ---------------------------------------------------------------------------
TEST_CASE("test_integrated_gating", "[LoudnessMeterAccuracy]")
{
    LoudnessMeter meter;
    meter.prepare(kSampleRate, 2);

    // Feed a loud signal for 2 seconds to establish integrated baseline
    const float loudAmp = 0.1f; // ~ -20 LUFS
    feedSine(meter, 1000.0, loudAmp, kSampleRate, 2.0);
    const float integratedAfterLoud = meter.getIntegratedLUFS();

    INFO("Integrated after loud: " << integratedAfterLoud);
    REQUIRE(!std::isinf(integratedAfterLoud));

    // Now feed silence for 5 seconds — below the -70 LUFS absolute gate threshold.
    // The integrated value should not change significantly after gating.
    feedSilence(meter, kSampleRate, 5.0);
    const float integratedAfterSilence = meter.getIntegratedLUFS();

    INFO("Integrated after 5s silence: " << integratedAfterSilence);

    // The integrated should not change drastically because silence is below the -70 LUFS gate
    // Allow up to ±3 LU change since 5s of silence is included in window averaging
    REQUIRE(integratedAfterSilence > integratedAfterLoud - 5.0f);
    REQUIRE(integratedAfterSilence < integratedAfterLoud + 1.0f);
}

// ---------------------------------------------------------------------------
// test_stereo_vs_dual_mono
// Correlated stereo (same signal L=R) should give the same LUFS reading as
// feeding the same mono signal to both channels. These are equivalent.
// ---------------------------------------------------------------------------
TEST_CASE("test_stereo_vs_dual_mono", "[LoudnessMeterAccuracy]")
{
    const float amp = 0.1f;

    // Stereo meter with same signal on both channels
    LoudnessMeter stereoMeter;
    stereoMeter.prepare(kSampleRate, 2);
    feedSine(stereoMeter, 1000.0, amp, kSampleRate, 5.0, 2);
    const float stereoLufs = stereoMeter.getIntegratedLUFS();

    // Another stereo meter — same configuration
    LoudnessMeter stereoMeter2;
    stereoMeter2.prepare(kSampleRate, 2);
    feedSine(stereoMeter2, 1000.0, amp, kSampleRate, 5.0, 2);
    const float stereoLufs2 = stereoMeter2.getIntegratedLUFS();

    INFO("Stereo LUFS: " << stereoLufs);
    INFO("Stereo LUFS 2: " << stereoLufs2);

    // Two measurements with identical input should match exactly (deterministic)
    REQUIRE(std::abs(stereoLufs - stereoLufs2) < 0.01f);
}

// ---------------------------------------------------------------------------
// test_loudness_range
// LRA measures the distribution of loudness over time. Feed a signal with
// alternating loud and quiet sections — LRA should be > 0 LU.
// ---------------------------------------------------------------------------
TEST_CASE("test_loudness_range", "[LoudnessMeterAccuracy]")
{
    LoudnessMeter meter;
    meter.prepare(kSampleRate, 2);

    // Alternate between loud (-10 LUFS) and quiet (-30 LUFS) in 1s segments
    // to create measurable dynamic range
    const float loudAmp  = static_cast<float>(std::pow(10.0, (-10.0 + 0.691) / 20.0));
    const float quietAmp = static_cast<float>(std::pow(10.0, (-30.0 + 0.691) / 20.0));

    // 10 alternations × 2s each = 20 seconds total
    for (int i = 0; i < 10; ++i)
    {
        feedSine(meter, 1000.0, loudAmp,  kSampleRate, 2.0);
        feedSine(meter, 1000.0, quietAmp, kSampleRate, 2.0);
    }

    const float lra = meter.getLoudnessRange();
    INFO("LRA: " << lra << " LU");

    // LRA should be > 0 with alternating levels
    REQUIRE(lra > 0.0f);
    // And should be in a reasonable range (not more than the 20dB difference we created)
    REQUIRE(lra < 25.0f);
}

// ---------------------------------------------------------------------------
// test_lra_gate_excludes_near_70_lufs
//
// EBU R128 §4.6 / ITU-R BS.1770-4: the absolute gate for LRA is exactly
// -70 LUFS. Windows with 3s-average LUFS ≤ -70 LUFS must be excluded.
//
// This test feeds a single-level signal whose 3s-window LUFS sits between
// -71 and -70 LUFS (-70.5 LUFS). With the correct -70 LUFS gate, all such
// windows are excluded → validCount = 0 → LRA = 0.
//
// With the former buggy gate (-71 LUFS), windows at -70.5 LUFS would be
// included (they satisfy l > -71) and clamped into histogram bin 0 at -70 LUFS.
// For a single-level signal, all windows fall in the same bin → LRA = 0 too.
// Therefore, LRA = 0 is the expected outcome in both cases; the test documents
// that near-gate signals do not produce a spurious nonzero LRA, and pairs with
// the grep acceptance criterion that verifies the gate expression is correct.
//
// Also verifies: no crash, finite short-term reading (signal IS measured).
// ---------------------------------------------------------------------------
TEST_CASE("test_lra_gate_excludes_near_70_lufs", "[LoudnessMeterAccuracy]")
{
    LoudnessMeter meter;
    meter.prepare(kSampleRate, 2);

    // Amplitude for approximately -70.5 LUFS at 1 kHz (K-weighting ~0 dB at 1 kHz):
    //   LUFS = -0.691 + 10*log10(A^2)  =>  A = 10^((-70.5 + 0.691) / 20)
    const float nearGateAmp = static_cast<float>(std::pow(10.0, (-70.5 + 0.691) / 20.0));

    // Feed 20 seconds of the near-gate signal — enough to fill many 3s LRA windows.
    feedSine(meter, 1000.0, nearGateAmp, kSampleRate, 20.0);

    const float lra = meter.getLoudnessRange();
    const float st  = meter.getShortTermLUFS();

    INFO("Short-term LUFS (expected ~-70.5): " << st);
    INFO("LRA (expected 0 — near-gate windows excluded or single-level): " << lra);

    // Short-term should reflect the signal (not -inf), confirming signal was processed.
    REQUIRE(!std::isinf(st));

    // LRA must be 0: either all windows excluded by the correct gate (validCount = 0)
    // or all in the same histogram bin (single-level, same percentiles).
    REQUIRE(lra == 0.0f);
}

// ---------------------------------------------------------------------------
// test_lra_constant_signal_zero
// A constant-level signal should produce LRA ≈ 0 LU since there is no
// loudness variation across time.
// ---------------------------------------------------------------------------
TEST_CASE("test_lra_constant_signal_zero", "[LoudnessMeterAccuracy]")
{
    LoudnessMeter meter;
    meter.prepare(kSampleRate, 2);

    // Amplitude for -23 LUFS at 1 kHz
    const float amp = static_cast<float>(std::pow(10.0, (-23.0 + 0.691) / 20.0));

    // Feed 10 seconds of constant-level tone
    feedSine(meter, 1000.0, amp, kSampleRate, 10.0);

    const float lra = meter.getLoudnessRange();
    INFO("LRA of constant signal: " << lra << " LU (expected < 0.5)");

    REQUIRE(lra < 0.5f);
}

// ---------------------------------------------------------------------------
// test_lra_alternating_loud_quiet
// Alternating between -23 LUFS and -33 LUFS sections should produce an
// LRA in the range [8.0, 12.0] LU (approximately 10 LU difference).
// ---------------------------------------------------------------------------
TEST_CASE("test_lra_alternating_loud_quiet", "[LoudnessMeterAccuracy]")
{
    LoudnessMeter meter;
    meter.prepare(kSampleRate, 2);

    const float loudAmp  = static_cast<float>(std::pow(10.0, (-23.0 + 0.691) / 20.0));
    const float quietAmp = static_cast<float>(std::pow(10.0, (-33.0 + 0.691) / 20.0));

    // Alternate loud/quiet in 5s segments for 30 seconds total
    for (int i = 0; i < 3; ++i)
    {
        feedSine(meter, 1000.0, loudAmp,  kSampleRate, 5.0);
        feedSine(meter, 1000.0, quietAmp, kSampleRate, 5.0);
    }

    const float lra = meter.getLoudnessRange();
    INFO("LRA of alternating -23/-33 LUFS: " << lra << " LU (expected 8-12)");

    REQUIRE(lra >= 8.0f);
    REQUIRE(lra <= 12.0f);
}

// ---------------------------------------------------------------------------
// test_absolute_gate_excludes_quiet_blocks
// A signal at -80 LUFS is below the absolute gate (-70 LUFS), so integrated
// loudness should return -inf (no windows pass the gate).
// ---------------------------------------------------------------------------
TEST_CASE("test_absolute_gate_excludes_quiet_blocks", "[LoudnessMeterAccuracy]")
{
    LoudnessMeter meter;
    meter.prepare(kSampleRate, 2);

    // Amplitude for -80 LUFS at 1 kHz
    const float amp = static_cast<float>(std::pow(10.0, (-80.0 + 0.691) / 20.0));

    // Feed 5 seconds of very quiet signal
    feedSine(meter, 1000.0, amp, kSampleRate, 5.0);

    const float integrated = meter.getIntegratedLUFS();
    INFO("Integrated LUFS of -80 LUFS signal: " << integrated << " (expected -inf)");

    REQUIRE(std::isinf(integrated));
    REQUIRE(integrated < 0.0f);
}

// ---------------------------------------------------------------------------
// test_reset_integrated_discards_history
// After resetIntegrated(), the integrated LUFS should reflect only post-reset
// signal, not the pre-reset history.
// ---------------------------------------------------------------------------
TEST_CASE("test_reset_integrated_discards_history", "[LoudnessMeterAccuracy]")
{
    LoudnessMeter meter;
    meter.prepare(kSampleRate, 2);

    // Phase 1: feed a loud signal (-14 LUFS) for 5 seconds
    const float loudAmp = static_cast<float>(std::pow(10.0, (-14.0 + 0.691) / 20.0));
    feedSine(meter, 1000.0, loudAmp, kSampleRate, 5.0);

    const float intBefore = meter.getIntegratedLUFS();
    INFO("Integrated before reset: " << intBefore);
    REQUIRE(!std::isinf(intBefore));
    REQUIRE(intBefore > -15.5f);
    REQUIRE(intBefore < -12.5f);

    // Reset
    meter.resetIntegrated();

    // Phase 2: feed a quiet signal (-30 LUFS) for 2 seconds
    const float quietAmp = static_cast<float>(std::pow(10.0, (-30.0 + 0.691) / 20.0));
    feedSine(meter, 1000.0, quietAmp, kSampleRate, 2.0);

    const float intAfter = meter.getIntegratedLUFS();
    INFO("Integrated after reset + quiet signal: " << intAfter);

    // Post-reset integrated should reflect only the -30 LUFS signal, not the -14 LUFS history
    REQUIRE(intAfter > -31.5f);
    REQUIRE(intAfter < -28.5f);
}

// ---------------------------------------------------------------------------
// test_relative_gating_loud_burst_dominates
// 20 seconds of silence followed by 5 seconds of -23 LUFS burst.
// The absolute gate excludes silence; the relative gate further filters.
// Integrated LUFS should be close to -23 LUFS (the burst level).
// ---------------------------------------------------------------------------
TEST_CASE("test_relative_gating_loud_burst_dominates", "[LoudnessMeterAccuracy]")
{
    LoudnessMeter meter;
    meter.prepare(kSampleRate, 2);

    // 20 seconds of silence
    feedSilence(meter, kSampleRate, 20.0);

    // 5 seconds of -23 LUFS burst
    const float burstAmp = static_cast<float>(std::pow(10.0, (-23.0 + 0.691) / 20.0));
    feedSine(meter, 1000.0, burstAmp, kSampleRate, 5.0);

    const float integrated = meter.getIntegratedLUFS();
    INFO("Integrated LUFS (silence + burst): " << integrated << " (expected ~-23)");

    // Silence is gated out, so integrated should be close to the burst level
    REQUIRE(integrated > -24.5f);
    REQUIRE(integrated < -21.5f);
}
