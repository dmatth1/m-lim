#include "catch2/catch_amalgamated.hpp"
#include "dsp/LoudnessMeter.h"
#include <juce_audio_basics/juce_audio_basics.h>
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
