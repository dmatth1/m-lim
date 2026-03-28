#include "catch2/catch_amalgamated.hpp"
#include "dsp/SidechainFilter.h"
#include <juce_audio_basics/juce_audio_basics.h>
#include <atomic>
#include <cmath>
#include <thread>
#include <vector>

static constexpr double kSampleRate   = 44100.0;
static constexpr double kTwoPi        = 6.283185307179586;
static constexpr int    kSettleSamples = 8192;  // let filters reach steady state
static constexpr int    kMeasureSamples = 8192; // samples used for RMS measurement

/** Generate a single-channel AudioBuffer filled with a sine wave. */
static juce::AudioBuffer<float> makeSine(double freq, double amplitude,
                                          int numSamples, double sampleRate)
{
    juce::AudioBuffer<float> buf(1, numSamples);
    float* data = buf.getWritePointer(0);
    for (int i = 0; i < numSamples; ++i)
        data[i] = static_cast<float>(amplitude * std::sin(kTwoPi * freq * i / sampleRate));
    return buf;
}

/** Compute RMS of a mono AudioBuffer. */
static double rms(const juce::AudioBuffer<float>& buf)
{
    const float* data = buf.getReadPointer(0);
    double sum = 0.0;
    for (int i = 0; i < buf.getNumSamples(); ++i)
        sum += static_cast<double>(data[i]) * data[i];
    return std::sqrt(sum / buf.getNumSamples());
}

// ---------------------------------------------------------------------------
// test_process_zero_sample_buffer_no_crash
// juce::AudioBuffer<float>(2, 0) creates a valid buffer with 0 samples.
// process() must handle it without crashing and must not corrupt the
// internal IIR filter state (a subsequent real buffer must produce finite output).
// ---------------------------------------------------------------------------
TEST_CASE("test_process_zero_sample_buffer_no_crash", "[SidechainFilter]")
{
    SidechainFilter filter;
    filter.prepare(kSampleRate, 512);

    // A buffer with 2 channels and 0 samples
    juce::AudioBuffer<float> zeroBuf(2, 0);
    REQUIRE_NOTHROW(filter.process(zeroBuf));

    // Verify internal state is intact by processing a real buffer next
    // and checking all output samples are finite.
    static constexpr int kBlockSize = 256;
    juce::AudioBuffer<float> realBuf(2, kBlockSize);
    for (int ch = 0; ch < 2; ++ch)
    {
        float* data = realBuf.getWritePointer(ch);
        for (int i = 0; i < kBlockSize; ++i)
            data[i] = static_cast<float>(std::sin(kTwoPi * 440.0 * i / kSampleRate));
    }

    filter.process(realBuf);

    for (int ch = 0; ch < 2; ++ch)
    {
        const float* out = realBuf.getReadPointer(ch);
        for (int i = 0; i < kBlockSize; ++i)
            REQUIRE(std::isfinite(out[i]));
    }
}

// ---------------------------------------------------------------------------
// test_highpass_attenuates_bass
// ---------------------------------------------------------------------------
TEST_CASE("test_highpass_attenuates_bass", "[SidechainFilter]")
{
    SidechainFilter filter;
    filter.prepare(kSampleRate, kSettleSamples + kMeasureSamples);
    filter.setHighPassFreq(100.0f);  // 100 Hz HP cutoff

    const int totalSamples = kSettleSamples + kMeasureSamples;
    auto buf = makeSine(50.0, 1.0, totalSamples, kSampleRate); // 50 Hz stimulus
    filter.process(buf);

    // Measure RMS in the latter portion (filter has settled)
    juce::AudioBuffer<float> measureBuf(1, kMeasureSamples);
    measureBuf.copyFrom(0, 0, buf, 0, kSettleSamples, kMeasureSamples);

    const double rmsOut = rms(measureBuf);
    const double rmsIn  = 1.0 / std::sqrt(2.0); // RMS of unit sine
    const double attenuationDb = 20.0 * std::log10(rmsOut / rmsIn);

    // A 2nd-order Butterworth HP at 100 Hz attenuates 50 Hz by ~12 dB.
    REQUIRE(attenuationDb < -6.0);
}

// ---------------------------------------------------------------------------
// test_lowpass_attenuates_treble
// ---------------------------------------------------------------------------
TEST_CASE("test_lowpass_attenuates_treble", "[SidechainFilter]")
{
    SidechainFilter filter;
    filter.prepare(kSampleRate, kSettleSamples + kMeasureSamples);
    filter.setLowPassFreq(5000.0f);  // 5 kHz LP cutoff

    const int totalSamples = kSettleSamples + kMeasureSamples;
    auto buf = makeSine(10000.0, 1.0, totalSamples, kSampleRate); // 10 kHz stimulus
    filter.process(buf);

    juce::AudioBuffer<float> measureBuf(1, kMeasureSamples);
    measureBuf.copyFrom(0, 0, buf, 0, kSettleSamples, kMeasureSamples);

    const double rmsOut = rms(measureBuf);
    const double rmsIn  = 1.0 / std::sqrt(2.0);
    const double attenuationDb = 20.0 * std::log10(rmsOut / rmsIn);

    // A 2nd-order Butterworth LP at 5 kHz attenuates 10 kHz by ~12 dB.
    REQUIRE(attenuationDb < -6.0);
}

// ---------------------------------------------------------------------------
// test_tilt_boosts_highs
// ---------------------------------------------------------------------------
TEST_CASE("test_tilt_boosts_highs", "[SidechainFilter]")
{
    // With a positive tilt, high-frequency content should have greater
    // amplitude than low-frequency content for the same input level.

    const int totalSamples = kSettleSamples + kMeasureSamples;

    auto runAtFreq = [&](double freq) -> double
    {
        SidechainFilter f;
        f.prepare(kSampleRate, totalSamples);
        f.setTilt(6.0f); // +6 dB tilt
        auto buf = makeSine(freq, 1.0, totalSamples, kSampleRate);
        f.process(buf);

        juce::AudioBuffer<float> measureBuf(1, kMeasureSamples);
        measureBuf.copyFrom(0, 0, buf, 0, kSettleSamples, kMeasureSamples);
        return rms(measureBuf);
    };

    const double rmsLow  = runAtFreq(100.0);   // well below 1 kHz pivot
    const double rmsHigh = runAtFreq(10000.0); // well above 1 kHz pivot

    // Positive tilt: high frequencies should be louder than low frequencies.
    REQUIRE(rmsHigh > rmsLow);
}

// ---------------------------------------------------------------------------
// test_flat_passes_signal
// ---------------------------------------------------------------------------
TEST_CASE("test_flat_passes_signal", "[SidechainFilter]")
{
    // Default settings (HP=20 Hz, LP=20 kHz, tilt=0) should be transparent
    // for a 1 kHz tone.

    SidechainFilter filter;
    filter.prepare(kSampleRate, kSettleSamples + kMeasureSamples);
    // No parameter changes — use defaults

    const int totalSamples = kSettleSamples + kMeasureSamples;
    auto buf = makeSine(1000.0, 1.0, totalSamples, kSampleRate);
    filter.process(buf);

    juce::AudioBuffer<float> measureBuf(1, kMeasureSamples);
    measureBuf.copyFrom(0, 0, buf, 0, kSettleSamples, kMeasureSamples);

    const double rmsOut = rms(measureBuf);
    const double rmsIn  = 1.0 / std::sqrt(2.0);

    // Attenuation at 1 kHz should be < 0.1 dB
    const double attenuationDb = 20.0 * std::log10(rmsOut / rmsIn);
    REQUIRE(std::abs(attenuationDb) < 0.1);
}

// ---------------------------------------------------------------------------
// test_negative_tilt_boosts_lows
// With tilt = -6 dB, low frequencies should be louder than high frequencies
// (opposite of positive tilt).
// ---------------------------------------------------------------------------
TEST_CASE("test_negative_tilt_boosts_lows", "[SidechainFilter]")
{
    const int totalSamples = kSettleSamples + kMeasureSamples;

    auto runAtFreq = [&](double freq) -> double
    {
        SidechainFilter f;
        f.prepare(kSampleRate, totalSamples);
        f.setTilt(-6.0f); // negative tilt: boost lows, cut highs
        auto buf = makeSine(freq, 1.0, totalSamples, kSampleRate);
        f.process(buf);

        juce::AudioBuffer<float> measureBuf(1, kMeasureSamples);
        measureBuf.copyFrom(0, 0, buf, 0, kSettleSamples, kMeasureSamples);
        return rms(measureBuf);
    };

    const double rmsLow  = runAtFreq(100.0);    // well below 1 kHz pivot
    const double rmsHigh = runAtFreq(10000.0);  // well above 1 kHz pivot

    // Negative tilt: low frequencies should be louder than high frequencies.
    REQUIRE(rmsLow > rmsHigh);
}

// ---------------------------------------------------------------------------
// test_extreme_params_no_crash
// HP=20 Hz, LP=20000 Hz (full bandwidth) — 100 blocks of sine → no crash,
// all output samples finite.
// ---------------------------------------------------------------------------
TEST_CASE("test_extreme_params_no_crash", "[SidechainFilter]")
{
    static constexpr int kBlockSize = 512;

    SidechainFilter filter;
    filter.prepare(kSampleRate, kBlockSize);
    filter.setHighPassFreq(20.0f);
    filter.setLowPassFreq(20000.0f);
    filter.setTilt(0.0f);

    juce::AudioBuffer<float> buf(1, kBlockSize);
    bool allFinite = true;

    for (int block = 0; block < 100; ++block)
    {
        float* data = buf.getWritePointer(0);
        for (int i = 0; i < kBlockSize; ++i)
            data[i] = static_cast<float>(std::sin(kTwoPi * 440.0 * i / kSampleRate));

        filter.process(buf);

        const float* out = buf.getReadPointer(0);
        for (int i = 0; i < kBlockSize; ++i)
            if (!std::isfinite(out[i])) allFinite = false;
    }

    REQUIRE(allFinite);
}

// ---------------------------------------------------------------------------
// test_hp_above_lp_no_crash
// HP=10000 Hz, LP=100 Hz (inverted — invalid but possible via param interaction).
// Must not crash, must not produce NaN/Inf.
// ---------------------------------------------------------------------------
TEST_CASE("test_hp_above_lp_no_crash", "[SidechainFilter]")
{
    static constexpr int kBlockSize = 512;

    SidechainFilter filter;
    filter.prepare(kSampleRate, kBlockSize);
    filter.setHighPassFreq(10000.0f);
    filter.setLowPassFreq(100.0f);

    juce::AudioBuffer<float> buf(1, kBlockSize);
    float* data = buf.getWritePointer(0);
    for (int i = 0; i < kBlockSize; ++i)
        data[i] = static_cast<float>(std::sin(kTwoPi * 1000.0 * i / kSampleRate));

    // Must not throw
    REQUIRE_NOTHROW(filter.process(buf));

    // Output must be finite
    const float* out = buf.getReadPointer(0);
    for (int i = 0; i < kBlockSize; ++i)
        REQUIRE(std::isfinite(out[i]));
}

// ---------------------------------------------------------------------------
// test_reprepare_different_sample_rates
// prepare(44100), process sine, prepare(96000), process sine → output finite
// both times. Verifies that re-prepare clears filter state correctly.
// ---------------------------------------------------------------------------
TEST_CASE("test_reprepare_different_sample_rates", "[SidechainFilter]")
{
    static constexpr int kBlockSize = 1024;

    SidechainFilter filter;

    auto runWithSampleRate = [&](double sr) -> bool
    {
        filter.prepare(sr, kBlockSize);
        filter.setHighPassFreq(100.0f);
        filter.setLowPassFreq(15000.0f);

        juce::AudioBuffer<float> buf(1, kBlockSize);
        float* data = buf.getWritePointer(0);
        for (int i = 0; i < kBlockSize; ++i)
            data[i] = static_cast<float>(std::sin(kTwoPi * 1000.0 * i / sr));

        filter.process(buf);

        const float* out = buf.getReadPointer(0);
        for (int i = 0; i < kBlockSize; ++i)
            if (!std::isfinite(out[i])) return false;
        return true;
    };

    REQUIRE(runWithSampleRate(44100.0));
    REQUIRE(runWithSampleRate(96000.0));
}

// ---------------------------------------------------------------------------
// test_hp_slope_at_least_6db_per_octave
// At HP=1000 Hz: RMS at 500 Hz (half-octave below) is at least 6 dB below
// RMS at 2000 Hz (one octave above). Checks that the HP filter rolls off
// at the correct rate.
// ---------------------------------------------------------------------------
TEST_CASE("test_hp_slope_at_least_6db_per_octave", "[SidechainFilter]")
{
    const int totalSamples = kSettleSamples + kMeasureSamples;

    auto measureRmsAt = [&](double freq) -> double
    {
        SidechainFilter f;
        f.prepare(kSampleRate, totalSamples);
        f.setHighPassFreq(1000.0f); // HP cutoff at 1 kHz
        auto buf = makeSine(freq, 1.0, totalSamples, kSampleRate);
        f.process(buf);

        juce::AudioBuffer<float> measureBuf(1, kMeasureSamples);
        measureBuf.copyFrom(0, 0, buf, 0, kSettleSamples, kMeasureSamples);
        return rms(measureBuf);
    };

    const double rms500  = measureRmsAt(500.0);   // one octave below cutoff
    const double rms2000 = measureRmsAt(2000.0);  // one octave above cutoff

    // HP filter: signal below cutoff attenuated, above should pass.
    // RMS at 500 Hz (stopped) should be at least 6 dB below 2000 Hz (passed).
    const double ratio_dB = 20.0 * std::log10(rms2000 / (rms500 + 1e-12));
    REQUIRE(ratio_dB > 6.0);
}

// ---------------------------------------------------------------------------
// test_parameter_change_during_process
// Calls setHighPassFreq() from a second thread while process() runs on the
// main thread. Verifies no crash and that the parameter change eventually
// takes effect (filter reflects the new HP cutoff).
// ---------------------------------------------------------------------------
TEST_CASE("test_parameter_change_during_process", "[SidechainFilter]")
{
    static constexpr int kBlockSize = 256;
    static constexpr int kNumBlocks = 200;

    SidechainFilter filter;
    filter.prepare(kSampleRate, kBlockSize);

    std::atomic<bool> stopWriter  { false };
    std::atomic<bool> writerReady { false };
    std::atomic<int>  writeCount  { 0 };

    // Writer thread: hammers setHighPassFreq() and setLowPassFreq() at a high
    // rate while the audio thread is calling process().
    std::thread writer([&]()
    {
        float hp = 20.0f;
        float lp = 20000.0f;
        writerReady.store(true, std::memory_order_release);
        while (!stopWriter.load(std::memory_order_relaxed))
        {
            filter.setHighPassFreq(hp);
            filter.setLowPassFreq(lp);
            filter.setTilt(0.0f);
            hp = (hp >= 500.0f) ? 20.0f : hp + 10.0f;
            lp = (lp <= 5000.0f) ? 20000.0f : lp - 100.0f;
            writeCount.fetch_add(1, std::memory_order_relaxed);
        }
    });

    // Wait for the writer thread to start before running the audio loop.
    while (!writerReady.load(std::memory_order_acquire)) {}

    // Audio thread: calls process() kNumBlocks times.
    juce::AudioBuffer<float> buf(1, kBlockSize);
    for (int block = 0; block < kNumBlocks; ++block)
    {
        // Fill with a 1 kHz sine
        float* data = buf.getWritePointer(0);
        for (int i = 0; i < kBlockSize; ++i)
            data[i] = static_cast<float>(std::sin(kTwoPi * 1000.0 * i / kSampleRate));

        filter.process(buf); // must not crash
    }

    stopWriter.store(true, std::memory_order_relaxed);
    writer.join();

    // Sanity: writer must have run at least once.
    REQUIRE(writeCount.load() > 0);

    // Now verify the parameter change actually took effect: set a 5 kHz HP
    // and confirm a 100 Hz tone is strongly attenuated after the next process().
    filter.setHighPassFreq(5000.0f);

    const int settleSamples  = 8192;
    const int measureSamples = 4096;
    juce::AudioBuffer<float> testBuf(1, settleSamples + measureSamples);
    float* data = testBuf.getWritePointer(0);
    for (int i = 0; i < settleSamples + measureSamples; ++i)
        data[i] = static_cast<float>(std::sin(kTwoPi * 100.0 * i / kSampleRate));

    filter.process(testBuf);

    // Measure RMS in the settled portion
    double sum = 0.0;
    for (int i = settleSamples; i < settleSamples + measureSamples; ++i)
        sum += static_cast<double>(data[i]) * data[i];
    const double rmsOut      = std::sqrt(sum / measureSamples);
    const double rmsIn       = 1.0 / std::sqrt(2.0);
    const double attenuationDb = 20.0 * std::log10(rmsOut / rmsIn + 1e-12);

    // 5 kHz HP attenuates 100 Hz by at least 20 dB (well below cutoff).
    REQUIRE(attenuationDb < -20.0);
}

// ---------------------------------------------------------------------------
// test_combined_hp_lp_passband
// HP at 80 Hz and LP at 8000 Hz active simultaneously:
//   (a) 50 Hz → attenuated > 6 dB  (below HP cutoff)
//   (b) 1 kHz → passes with < 1 dB attenuation (in the pass-band)
//   (c) 15 kHz → attenuated > 6 dB  (above LP cutoff)
// ---------------------------------------------------------------------------
TEST_CASE("test_combined_hp_lp_passband", "[SidechainFilter]")
{
    const int totalSamples = kSettleSamples + kMeasureSamples;

    // Helper: build a 2-channel buffer filled with the same sine on both channels.
    auto makeStereoSine = [&](double freq, double amplitude) -> juce::AudioBuffer<float>
    {
        juce::AudioBuffer<float> buf(2, totalSamples);
        for (int ch = 0; ch < 2; ++ch)
        {
            float* data = buf.getWritePointer(ch);
            for (int i = 0; i < totalSamples; ++i)
                data[i] = static_cast<float>(amplitude * std::sin(kTwoPi * freq * i / kSampleRate));
        }
        return buf;
    };

    // Helper: compute RMS of the settled portion from channel 0.
    auto settledRms = [&](const juce::AudioBuffer<float>& buf) -> double
    {
        const float* data = buf.getReadPointer(0, kSettleSamples);
        double sum = 0.0;
        for (int i = 0; i < kMeasureSamples; ++i)
            sum += static_cast<double>(data[i]) * data[i];
        return std::sqrt(sum / kMeasureSamples);
    };

    const double rmsIn = 1.0 / std::sqrt(2.0); // RMS of unit-amplitude sine

    // (a) 50 Hz — below HP cutoff at 80 Hz → should be attenuated > 6 dB
    {
        SidechainFilter filter;
        filter.prepare(kSampleRate, totalSamples);
        filter.setHighPassFreq(80.0f);
        filter.setLowPassFreq(8000.0f);

        auto buf = makeStereoSine(50.0, 1.0);
        filter.process(buf);

        const double rmsOut       = settledRms(buf);
        const double attenuationDb = 20.0 * std::log10(rmsOut / rmsIn + 1e-12);
        REQUIRE(attenuationDb < -6.0);
    }

    // (b) 1 kHz — within pass-band → should pass with < 1 dB attenuation
    {
        SidechainFilter filter;
        filter.prepare(kSampleRate, totalSamples);
        filter.setHighPassFreq(80.0f);
        filter.setLowPassFreq(8000.0f);

        auto buf = makeStereoSine(1000.0, 1.0);
        filter.process(buf);

        const double rmsOut       = settledRms(buf);
        const double attenuationDb = 20.0 * std::log10(rmsOut / rmsIn + 1e-12);
        REQUIRE(attenuationDb > -1.0);
    }

    // (c) 15 kHz — above LP cutoff at 8 kHz → should be attenuated > 6 dB
    {
        SidechainFilter filter;
        filter.prepare(kSampleRate, totalSamples);
        filter.setHighPassFreq(80.0f);
        filter.setLowPassFreq(8000.0f);

        auto buf = makeStereoSine(15000.0, 1.0);
        filter.process(buf);

        const double rmsOut       = settledRms(buf);
        const double attenuationDb = 20.0 * std::log10(rmsOut / rmsIn + 1e-12);
        REQUIRE(attenuationDb < -6.0);
    }
}

// ---------------------------------------------------------------------------
// test_setter_clamping_no_crash
// Calls each setter with values well outside the valid range, verifies no
// crash and that all output samples remain finite (the clamp prevents
// degenerate bilinear-transform coefficients from producing NaN/Inf).
// ---------------------------------------------------------------------------
TEST_CASE("test_setter_clamping_no_crash", "[SidechainFilter]")
{
    static constexpr int kBlockSize = 256;

    auto allFiniteAfterProcess = [&](SidechainFilter& f) -> bool
    {
        juce::AudioBuffer<float> buf(1, kBlockSize);
        float* data = buf.getWritePointer(0);
        for (int i = 0; i < kBlockSize; ++i)
            data[i] = static_cast<float>(std::sin(kTwoPi * 1000.0 * i / kSampleRate));

        REQUIRE_NOTHROW(f.process(buf));

        const float* out = buf.getReadPointer(0);
        for (int i = 0; i < kBlockSize; ++i)
            if (!std::isfinite(out[i])) return false;
        return true;
    };

    // --- setHighPassFreq out of range ---
    {
        SidechainFilter f;
        f.prepare(kSampleRate, kBlockSize);
        f.setHighPassFreq(-1.0f);    // below minimum (20 Hz) → clamped to 20 Hz
        REQUIRE(allFiniteAfterProcess(f));
    }
    {
        SidechainFilter f;
        f.prepare(kSampleRate, kBlockSize);
        f.setHighPassFreq(99999.0f); // above maximum (2000 Hz) → clamped to 2000 Hz
        REQUIRE(allFiniteAfterProcess(f));
    }

    // --- setLowPassFreq out of range ---
    {
        SidechainFilter f;
        f.prepare(kSampleRate, kBlockSize);
        f.setLowPassFreq(-1.0f);     // below minimum (2000 Hz) → clamped to 2000 Hz
        REQUIRE(allFiniteAfterProcess(f));
    }
    {
        SidechainFilter f;
        f.prepare(kSampleRate, kBlockSize);
        f.setLowPassFreq(99999.0f);  // above maximum (20000 Hz) → clamped to 20000 Hz
        REQUIRE(allFiniteAfterProcess(f));
    }

    // --- setTilt out of range ---
    {
        SidechainFilter f;
        f.prepare(kSampleRate, kBlockSize);
        f.setTilt(-100.0f);          // below minimum (-6 dB) → clamped to -6 dB
        REQUIRE(allFiniteAfterProcess(f));
    }
    {
        SidechainFilter f;
        f.prepare(kSampleRate, kBlockSize);
        f.setTilt(100.0f);           // above maximum (+6 dB) → clamped to +6 dB
        REQUIRE(allFiniteAfterProcess(f));
    }
}

// ---------------------------------------------------------------------------
// test_coefficient_change_no_transient_spike
// Processes a block at HP=200 Hz, then changes HP to 1000 Hz, processes the
// next block, and verifies no sample in that next block exceeds 2× the input
// amplitude (i.e. no transient spike from stale filter state).
// ---------------------------------------------------------------------------
TEST_CASE("test_coefficient_change_no_transient_spike", "[SidechainFilter]")
{
    static constexpr int    kBlockSize   = 512;
    static constexpr double kInputFreq   = 1000.0;  // 1 kHz sine, passes both HP settings
    static constexpr float  kAmplitude   = 0.5f;
    static constexpr float  kSpikeCeiling = 2.0f * kAmplitude;  // 2× input ceiling

    SidechainFilter filter;
    filter.prepare(kSampleRate, kBlockSize);

    // Set initial HP at 200 Hz and let the filter settle.
    filter.setHighPassFreq(200.0f);

    // Process several settling blocks so the IIR delay state reaches steady-state.
    juce::AudioBuffer<float> settleBuf(1, kBlockSize);
    for (int block = 0; block < 20; ++block)
    {
        float* data = settleBuf.getWritePointer(0);
        for (int i = 0; i < kBlockSize; ++i)
            data[i] = kAmplitude * static_cast<float>(std::sin(kTwoPi * kInputFreq * i / kSampleRate));
        filter.process(settleBuf);
    }

    // Now change the HP frequency — this should trigger a coefficient update
    // and reset the filter state on the next process() call.
    filter.setHighPassFreq(1000.0f);

    // Process one block immediately after the frequency change.
    juce::AudioBuffer<float> testBuf(1, kBlockSize);
    float* data = testBuf.getWritePointer(0);
    for (int i = 0; i < kBlockSize; ++i)
        data[i] = kAmplitude * static_cast<float>(std::sin(kTwoPi * kInputFreq * i / kSampleRate));

    filter.process(testBuf);

    // Check that no sample in the block after the change exceeds 2× the input amplitude.
    const float* out = testBuf.getReadPointer(0);
    bool noSpike = true;
    for (int i = 0; i < kBlockSize; ++i)
    {
        if (std::abs(out[i]) > kSpikeCeiling)
        {
            noSpike = false;
            break;
        }
    }

    REQUIRE(noSpike);
}

// ---------------------------------------------------------------------------
// test_inplace_coefficients_correctness
// Verifies that after the RT-safe coefficient update path (in-place writes to
// pre-allocated Coefficients objects), the filter output is consistent with
// the expected filter behaviour:
//   - HP at 1 kHz strongly attenuates 100 Hz (at least 12 dB).
//   - LP at 5 kHz strongly attenuates 10 kHz (at least 12 dB).
//   - Repeated calls to setHighPassFreq/setLowPassFreq/setTilt followed by
//     process() do not crash and all outputs remain finite.
// This exercises updateCoefficients() via the in-place write path (called
// after prepare() has pre-allocated the Coefficients objects).
// ---------------------------------------------------------------------------
TEST_CASE("test_inplace_coefficients_correctness", "[SidechainFilter]")
{
    static constexpr int kBlock     = 256;
    static constexpr int kSettle    = 8192;
    static constexpr int kMeasure   = 4096;
    static constexpr int kTotal     = kSettle + kMeasure;

    // -- 1. HP correctness at 1 kHz: 100 Hz should be attenuated >= 12 dB --
    {
        SidechainFilter f;
        f.prepare(kSampleRate, kTotal);
        f.setHighPassFreq(1000.0f);   // trigger updateCoefficients() on next process()

        auto buf = makeSine(100.0, 1.0, kTotal, kSampleRate);
        f.process(buf);

        juce::AudioBuffer<float> measure(1, kMeasure);
        measure.copyFrom(0, 0, buf, 0, kSettle, kMeasure);

        const double rmsOut = rms(measure);
        const double rmsIn  = 1.0 / std::sqrt(2.0);
        const double atten  = 20.0 * std::log10(rmsOut / rmsIn + 1e-12);
        REQUIRE(atten < -12.0);
    }

    // -- 2. LP correctness at 5 kHz: 10 kHz should be attenuated >= 12 dB --
    {
        SidechainFilter f;
        f.prepare(kSampleRate, kTotal);
        f.setLowPassFreq(5000.0f);

        auto buf = makeSine(10000.0, 1.0, kTotal, kSampleRate);
        f.process(buf);

        juce::AudioBuffer<float> measure(1, kMeasure);
        measure.copyFrom(0, 0, buf, 0, kSettle, kMeasure);

        const double rmsOut = rms(measure);
        const double rmsIn  = 1.0 / std::sqrt(2.0);
        const double atten  = 20.0 * std::log10(rmsOut / rmsIn + 1e-12);
        REQUIRE(atten < -12.0);
    }

    // -- 3. Multiple rapid parameter changes: no crash, all output finite --
    {
        SidechainFilter f;
        f.prepare(kSampleRate, kBlock);

        juce::AudioBuffer<float> buf(1, kBlock);
        bool allFinite = true;

        const float hpValues[]  = { 20.0f, 100.0f, 500.0f, 1000.0f, 2000.0f };
        const float lpValues[]  = { 20000.0f, 15000.0f, 10000.0f, 5000.0f };
        const float tiltValues[] = { 0.0f, 3.0f, -3.0f, 6.0f, -6.0f };

        int combo = 0;
        for (float hp : hpValues)
        for (float lp : lpValues)
        for (float tilt : tiltValues)
        {
            f.setHighPassFreq(hp);
            f.setLowPassFreq(lp);
            f.setTilt(tilt);

            float* data = buf.getWritePointer(0);
            for (int i = 0; i < kBlock; ++i)
                data[i] = static_cast<float>(
                    std::sin(kTwoPi * 1000.0 * (combo * kBlock + i) / kSampleRate));

            f.process(buf);

            const float* out = buf.getReadPointer(0);
            for (int i = 0; i < kBlock; ++i)
                if (!std::isfinite(out[i])) allFinite = false;

            ++combo;
        }

        REQUIRE(allFinite);
    }
}

// ---------------------------------------------------------------------------
// test_hp_cutoff_change_no_discontinuity
// Verifies that changing the HP cutoff mid-signal does not cause a hard reset
// (step to near-zero) in the filter output.  Before the fix, updateCoefficients()
// called reset() on all IIR states, producing a zero-discontinuity that caused
// false GR transients.  With the fix the filter state persists and the output
// at the sample immediately after the coefficient change must remain close to
// the output just before it (within ±0.1).
// ---------------------------------------------------------------------------
TEST_CASE("test_hp_cutoff_change_no_discontinuity", "[SidechainFilter]")
{
    SidechainFilter filter;
    filter.prepare(kSampleRate, 512);

    // Start with a 100 Hz HP cutoff.
    filter.setHighPassFreq(100.0f);

    // Build up filter state: process 512 samples of 1 kHz sine.
    auto buildUp = makeSine(1000.0, 0.5, 512, kSampleRate);
    filter.process(buildUp);

    // Process one more sample to capture the last output value before the change.
    auto beforeBuf = makeSine(1000.0, 0.5, 1, kSampleRate);
    filter.process(beforeBuf);
    const float lastBefore = beforeBuf.getReadPointer(0)[0];

    // Change HP cutoff to 200 Hz — this triggers updateCoefficients().
    filter.setHighPassFreq(200.0f);

    // Process one sample after the change.
    auto afterBuf = makeSine(1000.0, 0.5, 1, kSampleRate);
    filter.process(afterBuf);
    const float firstAfter = afterBuf.getReadPointer(0)[0];

    // The output must not jump discontinuously (i.e. not reset to near-zero).
    // A hard state reset would push firstAfter close to 0 while lastBefore is ~0.5.
    const float jump = std::abs(firstAfter - lastBefore);
    REQUIRE(jump < 0.1f);
}

// ---------------------------------------------------------------------------
// test_inverted_hp_lp_long_run_stable
// HP=2000, LP=200 (inverted range), 100k samples of white noise — all output
// must remain finite and within [-10, +10].
// ---------------------------------------------------------------------------
TEST_CASE("test_inverted_hp_lp_long_run_stable", "[SidechainFilter]")
{
    static constexpr int kBlockSize   = 512;
    static constexpr int kNumBlocks   = 200; // 200 * 512 = 102400 samples

    SidechainFilter filter;
    filter.prepare(kSampleRate, kBlockSize);
    filter.setHighPassFreq(2000.0f);
    filter.setLowPassFreq(2000.0f); // LP clamps to min 2000 Hz

    juce::AudioBuffer<float> buf(1, kBlockSize);
    juce::Random rng(42);

    for (int block = 0; block < kNumBlocks; ++block)
    {
        float* data = buf.getWritePointer(0);
        for (int i = 0; i < kBlockSize; ++i)
            data[i] = rng.nextFloat() * 2.0f - 1.0f; // white noise [-1, 1]

        filter.process(buf);

        const float* out = buf.getReadPointer(0);
        for (int i = 0; i < kBlockSize; ++i)
        {
            REQUIRE(std::isfinite(out[i]));
            REQUIRE(std::abs(out[i]) <= 10.0f);
        }
    }
}

// ---------------------------------------------------------------------------
// test_hp_sweep_no_explosion
// HP sweeps from 20 to 2000 Hz in steps, 100 samples per step; peak output
// must stay below 40 dBFS (roughly amplitude 100).
// ---------------------------------------------------------------------------
TEST_CASE("test_hp_sweep_no_explosion", "[SidechainFilter]")
{
    static constexpr int kSamplesPerStep = 100;
    static constexpr float kMaxAmplitude = 100.0f; // ~40 dBFS

    SidechainFilter filter;
    filter.prepare(kSampleRate, kSamplesPerStep);

    juce::AudioBuffer<float> buf(1, kSamplesPerStep);
    int sampleIndex = 0;

    for (float hp = 20.0f; hp <= 2000.0f; hp += 20.0f)
    {
        filter.setHighPassFreq(hp);

        float* data = buf.getWritePointer(0);
        for (int i = 0; i < kSamplesPerStep; ++i)
            data[i] = static_cast<float>(std::sin(kTwoPi * 1000.0 * (sampleIndex + i) / kSampleRate));

        filter.process(buf);

        const float* out = buf.getReadPointer(0);
        for (int i = 0; i < kSamplesPerStep; ++i)
        {
            REQUIRE(std::isfinite(out[i]));
            REQUIRE(std::abs(out[i]) < kMaxAmplitude);
        }

        sampleIndex += kSamplesPerStep;
    }
}

// ---------------------------------------------------------------------------
// test_reprepare_clears_filter_state
// Prime the filter with tilt=+6 dB and 10k loud samples, then reprepare and
// process 10 silence samples — output must be within 1e-4 of 0.0.
// ---------------------------------------------------------------------------
TEST_CASE("test_reprepare_clears_filter_state", "[SidechainFilter]")
{
    SidechainFilter filter;
    filter.prepare(kSampleRate, 10000);
    filter.setTilt(6.0f);

    // Prime with loud signal
    juce::AudioBuffer<float> loudBuf(1, 10000);
    {
        float* data = loudBuf.getWritePointer(0);
        for (int i = 0; i < 10000; ++i)
            data[i] = static_cast<float>(std::sin(kTwoPi * 1000.0 * i / kSampleRate));
    }
    filter.process(loudBuf);

    // Reprepare — should clear all filter state
    filter.prepare(kSampleRate, 10);

    // Process 10 silence samples
    juce::AudioBuffer<float> silenceBuf(1, 10);
    silenceBuf.clear();
    filter.process(silenceBuf);

    const float* out = silenceBuf.getReadPointer(0);
    for (int i = 0; i < 10; ++i)
    {
        REQUIRE(std::abs(out[i]) < 1e-4f);
    }
}

// ---------------------------------------------------------------------------
// test_tilt_unity_at_pivot_frequency
// At the 1 kHz pivot with tilt=+6 dB, the combined low-shelf + high-shelf
// should be near unity. Measure attenuation at 1 kHz — must be within ±0.5 dB.
// ---------------------------------------------------------------------------
TEST_CASE("test_tilt_unity_at_pivot_frequency", "[SidechainFilter]")
{
    const int totalSamples = kSettleSamples + kMeasureSamples;

    SidechainFilter filter;
    filter.prepare(kSampleRate, totalSamples);
    filter.setTilt(6.0f); // +6 dB tilt

    auto buf = makeSine(1000.0, 1.0, totalSamples, kSampleRate);
    filter.process(buf);

    juce::AudioBuffer<float> measureBuf(1, kMeasureSamples);
    measureBuf.copyFrom(0, 0, buf, 0, kSettleSamples, kMeasureSamples);

    const double rmsOut = rms(measureBuf);
    const double rmsIn  = 1.0 / std::sqrt(2.0);
    const double attenuationDb = 20.0 * std::log10(rmsOut / rmsIn);

    REQUIRE(attenuationDb > -0.5);
    REQUIRE(attenuationDb < 0.5);
}

// ---------------------------------------------------------------------------
// test_8khz_sample_rate_stable
//   HP=2000Hz is near Nyquist at 8kHz — verify no NaN or explosion.
// ---------------------------------------------------------------------------
TEST_CASE("test_8khz_sample_rate_stable", "[SidechainFilter]")
{
    const double sampleRate = 8000.0;
    const int numSamples = 4096;

    SidechainFilter filter;
    filter.prepare(sampleRate, numSamples);
    filter.setHighPassFreq(2000.0f); // near Nyquist (4kHz)

    auto buf = makeSine(1000.0, 1.0, numSamples, sampleRate);
    filter.process(buf);

    // All output samples must be finite (no NaN or Inf)
    const float* data = buf.getReadPointer(0);
    for (int i = 0; i < numSamples; ++i)
    {
        INFO("i=" << i << " val=" << data[i]);
        REQUIRE(std::isfinite(data[i]));
    }
}

// ---------------------------------------------------------------------------
// test_384khz_sample_rate_stable
//   Very high sample rate — verify stability and finite output.
// ---------------------------------------------------------------------------
TEST_CASE("test_384khz_sample_rate_stable", "[SidechainFilter]")
{
    const double sampleRate = 384000.0;
    const int numSamples = 8192;

    SidechainFilter filter;
    filter.prepare(sampleRate, numSamples);
    filter.setHighPassFreq(200.0f);
    filter.setLowPassFreq(15000.0f);

    auto buf = makeSine(1000.0, 1.0, numSamples, sampleRate);
    filter.process(buf);

    // All output samples must be finite
    const float* data = buf.getReadPointer(0);
    for (int i = 0; i < numSamples; ++i)
    {
        INFO("i=" << i << " val=" << data[i]);
        REQUIRE(std::isfinite(data[i]));
    }

    // RMS should be reasonable (1kHz is well within passband)
    double sum = 0.0;
    for (int i = numSamples / 2; i < numSamples; ++i)
        sum += static_cast<double>(data[i]) * data[i];
    double measuredRms = std::sqrt(sum / (numSamples / 2));
    REQUIRE(measuredRms > 0.01); // signal should pass through
}
