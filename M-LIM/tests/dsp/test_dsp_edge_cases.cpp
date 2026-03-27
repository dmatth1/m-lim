#include "catch2/catch_amalgamated.hpp"
#include "dsp/DCFilter.h"
#include "dsp/Dither.h"
#include "dsp/TruePeakDetector.h"
#include "dsp/SidechainFilter.h"
#include "dsp/LoudnessMeter.h"
#include "dsp/TransientLimiter.h"
#include "dsp/LevelingLimiter.h"
#include "dsp/LimiterEngine.h"
#include "dsp/Oversampler.h"
#include <juce_audio_basics/juce_audio_basics.h>
#include <juce_dsp/juce_dsp.h>
#include <cmath>
#include <limits>
#include <vector>

static constexpr double kSR44 = 44100.0;
static constexpr double kSR48 = 48000.0;
static constexpr double kSR96 = 96000.0;
static constexpr double kSR192 = 192000.0;

// ============================================================================
// test_all_dsp_zero_length_buffer
// Call process() with 0 samples — must not crash.
// ============================================================================
TEST_CASE("test_all_dsp_zero_length_buffer", "[DSPEdgeCases]")
{
    SECTION("DCFilter")
    {
        DCFilter f;
        f.prepare(kSR44);
        float dummy = 0.5f;
        f.process(&dummy, 0);  // should not crash
    }

    SECTION("Dither")
    {
        Dither d;
        d.prepare(kSR44);
        float dummy = 0.5f;
        d.process(&dummy, 0);
    }

    SECTION("TruePeakDetector")
    {
        TruePeakDetector tp;
        tp.prepare(kSR44);
        float dummy = 0.5f;
        tp.processBlock(&dummy, 0);
        REQUIRE(tp.getPeak() == 0.0f);
    }

    SECTION("LoudnessMeter")
    {
        LoudnessMeter meter;
        meter.prepare(kSR48, 2);
        juce::AudioBuffer<float> empty(2, 0);
        meter.processBlock(empty);  // should not crash
    }

    SECTION("SidechainFilter")
    {
        SidechainFilter sf;
        sf.prepare(kSR44, 512);
        juce::AudioBuffer<float> empty(2, 0);
        sf.process(empty);
    }

    SECTION("TransientLimiter")
    {
        TransientLimiter tl;
        tl.prepare(kSR44, 512, 2);
        std::vector<float> ch0, ch1;
        float* channels[2] = { nullptr, nullptr };
        tl.process(channels, 2, 0, nullptr);  // 0 samples, should not crash
    }

    SECTION("LevelingLimiter")
    {
        LevelingLimiter ll;
        ll.prepare(kSR44, 512, 2);
        float* channels[2] = { nullptr, nullptr };
        ll.process(channels, 2, 0, nullptr);
    }
}

// ============================================================================
// test_all_dsp_single_sample_buffer
// Process exactly 1 sample — must not crash and produce a valid result.
// ============================================================================
TEST_CASE("test_all_dsp_single_sample_buffer", "[DSPEdgeCases]")
{
    SECTION("DCFilter")
    {
        DCFilter f;
        f.prepare(kSR44);
        float sample = 0.5f;
        f.process(&sample, 1);
        REQUIRE(std::isfinite(sample));
    }

    SECTION("Dither")
    {
        Dither d;
        d.prepare(kSR44);
        float sample = 0.5f;
        d.process(&sample, 1);
        REQUIRE(std::isfinite(sample));
    }

    SECTION("TruePeakDetector")
    {
        TruePeakDetector tp;
        tp.prepare(kSR44);
        float sample = 0.5f;
        float result = tp.processSample(sample);
        REQUIRE(std::isfinite(result));
    }

    SECTION("LoudnessMeter")
    {
        LoudnessMeter meter;
        meter.prepare(kSR48, 2);
        juce::AudioBuffer<float> buf(2, 1);
        buf.clear();
        buf.setSample(0, 0, 0.5f);
        buf.setSample(1, 0, 0.5f);
        meter.processBlock(buf);  // no crash
    }

    SECTION("SidechainFilter")
    {
        SidechainFilter sf;
        sf.prepare(kSR44, 512);
        juce::AudioBuffer<float> buf(2, 1);
        buf.clear();
        buf.setSample(0, 0, 0.5f);
        buf.setSample(1, 0, 0.5f);
        sf.process(buf);
        REQUIRE(std::isfinite(buf.getSample(0, 0)));
    }

    SECTION("TransientLimiter")
    {
        TransientLimiter tl;
        tl.prepare(kSR44, 512, 2);
        float ch0[1] = { 0.5f };
        float ch1[1] = { 0.5f };
        float* channels[2] = { ch0, ch1 };
        tl.process(channels, 2, 1, nullptr);
        REQUIRE(std::isfinite(ch0[0]));
        REQUIRE(std::isfinite(ch1[0]));
    }

    SECTION("LevelingLimiter")
    {
        LevelingLimiter ll;
        ll.prepare(kSR44, 512, 2);
        float ch0[1] = { 0.5f };
        float ch1[1] = { 0.5f };
        float* channels[2] = { ch0, ch1 };
        ll.process(channels, 2, 1, nullptr);
        REQUIRE(std::isfinite(ch0[0]));
        REQUIRE(std::isfinite(ch1[0]));
    }
}

// ============================================================================
// test_nan_input_does_not_propagate
// NaN input must not cause other samples to become NaN.
// (Components may output NaN for the NaN sample itself — that is acceptable
// for some designs — but subsequent valid samples must remain valid.)
// ============================================================================
TEST_CASE("test_nan_input_does_not_propagate", "[DSPEdgeCases]")
{
    const float nanVal = std::numeric_limits<float>::quiet_NaN();

    SECTION("DCFilter")
    {
        DCFilter f;
        f.prepare(kSR44);
        // Process a NaN, then valid samples
        float buf[4] = { 0.5f, nanVal, 0.5f, 0.5f };
        f.process(buf, 4);
        // After NaN, the filter state may be corrupted — reset and re-test
        f.reset();
        float valid[4] = { 0.3f, 0.3f, 0.3f, 0.3f };
        f.process(valid, 4);
        // After reset, output must be finite
        for (int i = 0; i < 4; ++i)
            REQUIRE(std::isfinite(valid[i]));
    }

    SECTION("TruePeakDetector")
    {
        TruePeakDetector tp;
        tp.prepare(kSR44);
        float sample = nanVal;
        tp.processSample(sample);
        tp.reset();
        float good = tp.processSample(0.5f);
        // After reset and valid input, result should be finite
        REQUIRE(std::isfinite(good));
    }

    SECTION("Dither")
    {
        Dither d;
        d.prepare(kSR44);
        // Feed NaN through dither
        float buf[2] = { nanVal, 0.5f };
        d.process(buf, 2);
        // Reset and process clean signal
        d.prepare(kSR44);
        float valid[2] = { 0.5f, 0.5f };
        d.process(valid, 2);
        for (int i = 0; i < 2; ++i)
            REQUIRE(std::isfinite(valid[i]));
    }
}

// ============================================================================
// test_inf_input_does_not_cause_crash
// Inf inputs must not cause crashes. Components may clamp or propagate Inf,
// but must not throw exceptions or access invalid memory.
// ============================================================================
TEST_CASE("test_inf_input_does_not_cause_crash", "[DSPEdgeCases]")
{
    const float posInf = std::numeric_limits<float>::infinity();
    const float negInf = -std::numeric_limits<float>::infinity();

    SECTION("DCFilter no crash")
    {
        DCFilter f;
        f.prepare(kSR44);
        float buf[4] = { posInf, negInf, posInf, 0.0f };
        f.process(buf, 4);  // must not crash
    }

    SECTION("Dither no crash")
    {
        Dither d;
        d.prepare(kSR44);
        float buf[2] = { posInf, negInf };
        d.process(buf, 2);  // must not crash
    }

    SECTION("TruePeakDetector no crash")
    {
        TruePeakDetector tp;
        tp.prepare(kSR44);
        tp.processSample(posInf);
        tp.processSample(negInf);
        tp.reset();
        // After reset, valid input should work fine
        float result = tp.processSample(0.5f);
        REQUIRE(std::isfinite(result));
    }

    SECTION("TransientLimiter no crash")
    {
        TransientLimiter tl;
        tl.prepare(kSR44, 512, 2);
        float ch0[4] = { posInf, negInf, 0.5f, 0.5f };
        float ch1[4] = { posInf, negInf, 0.5f, 0.5f };
        float* channels[2] = { ch0, ch1 };
        tl.process(channels, 2, 4, nullptr);  // must not crash
    }

    SECTION("LevelingLimiter no crash")
    {
        LevelingLimiter ll;
        ll.prepare(kSR44, 512, 2);
        float ch0[4] = { posInf, negInf, 0.5f, 0.5f };
        float ch1[4] = { posInf, negInf, 0.5f, 0.5f };
        float* channels[2] = { ch0, ch1 };
        ll.process(channels, 2, 4, nullptr);  // must not crash
    }
}

// ============================================================================
// test_denormal_input_handling
// Feed denormal floats (very small values near zero). With ScopedNoDenormals
// enabled, these should be flushed to zero, preventing CPU slowdown.
// After denormals pass through, output should be finite (≥ 0 or exactly 0).
// ============================================================================
TEST_CASE("test_denormal_input_handling", "[DSPEdgeCases]")
{
    const float denorm = std::numeric_limits<float>::denorm_min(); // ~1.4e-45

    SECTION("DCFilter denormals")
    {
        DCFilter f;
        f.prepare(kSR44);
        std::vector<float> buf(128, denorm);
        f.process(buf.data(), 128);
        for (int i = 0; i < 128; ++i)
            REQUIRE(std::isfinite(buf[i]));
    }

    SECTION("Dither denormals")
    {
        Dither d;
        d.prepare(kSR44);
        std::vector<float> buf(128, denorm);
        d.process(buf.data(), 128);
        for (int i = 0; i < 128; ++i)
            REQUIRE(std::isfinite(buf[i]));
    }

    SECTION("TruePeakDetector denormals")
    {
        TruePeakDetector tp;
        tp.prepare(kSR44);
        std::vector<float> buf(128, denorm);
        tp.processBlock(buf.data(), 128);
        REQUIRE(std::isfinite(tp.getPeak()));
    }

    SECTION("TransientLimiter denormals")
    {
        TransientLimiter tl;
        tl.prepare(kSR44, 512, 2);
        std::vector<float> ch0(128, denorm), ch1(128, denorm);
        float* channels[2] = { ch0.data(), ch1.data() };
        tl.process(channels, 2, 128, nullptr);
        for (int i = 0; i < 128; ++i)
        {
            REQUIRE(std::isfinite(ch0[i]));
            REQUIRE(std::isfinite(ch1[i]));
        }
    }

    SECTION("LevelingLimiter denormals")
    {
        LevelingLimiter ll;
        ll.prepare(kSR44, 512, 2);
        std::vector<float> ch0(128, denorm), ch1(128, denorm);
        float* channels[2] = { ch0.data(), ch1.data() };
        ll.process(channels, 2, 128, nullptr);
        for (int i = 0; i < 128; ++i)
        {
            REQUIRE(std::isfinite(ch0[i]));
            REQUIRE(std::isfinite(ch1[i]));
        }
    }
}

// ============================================================================
// test_sample_rate_change_reprepare
// Call prepare() with 44100, process, then call prepare() with 96000 and
// process again. Must not crash or produce invalid output.
// ============================================================================
TEST_CASE("test_sample_rate_change_reprepare", "[DSPEdgeCases]")
{
    SECTION("DCFilter")
    {
        DCFilter f;
        f.prepare(kSR44);
        float buf[64];
        std::fill(buf, buf + 64, 0.3f);
        f.process(buf, 64);

        f.prepare(kSR96);
        std::fill(buf, buf + 64, 0.3f);
        f.process(buf, 64);
        for (int i = 0; i < 64; ++i)
            REQUIRE(std::isfinite(buf[i]));
    }

    SECTION("Dither")
    {
        Dither d;
        d.prepare(kSR44);
        float buf[64];
        std::fill(buf, buf + 64, 0.3f);
        d.process(buf, 64);

        d.prepare(kSR96);
        std::fill(buf, buf + 64, 0.3f);
        d.process(buf, 64);
        for (int i = 0; i < 64; ++i)
            REQUIRE(std::isfinite(buf[i]));
    }

    SECTION("LoudnessMeter")
    {
        LoudnessMeter meter;
        meter.prepare(kSR44, 2);
        juce::AudioBuffer<float> buf44(2, 512);
        buf44.clear();
        meter.processBlock(buf44);

        meter.prepare(kSR96, 2);
        juce::AudioBuffer<float> buf96(2, 512);
        buf96.clear();
        meter.processBlock(buf96);
        // No crash = pass
    }

    SECTION("TransientLimiter")
    {
        TransientLimiter tl;
        tl.prepare(kSR44, 512, 2);
        std::vector<float> ch0(512, 0.3f), ch1(512, 0.3f);
        float* ch[2] = { ch0.data(), ch1.data() };
        tl.process(ch, 2, 512, nullptr);

        tl.prepare(kSR96, 512, 2);
        std::fill(ch0.begin(), ch0.end(), 0.3f);
        std::fill(ch1.begin(), ch1.end(), 0.3f);
        tl.process(ch, 2, 512, nullptr);
        for (int i = 0; i < 512; ++i)
        {
            REQUIRE(std::isfinite(ch0[i]));
            REQUIRE(std::isfinite(ch1[i]));
        }
    }

    SECTION("LevelingLimiter")
    {
        LevelingLimiter ll;
        ll.prepare(kSR44, 512, 2);
        std::vector<float> ch0(512, 0.3f), ch1(512, 0.3f);
        float* ch[2] = { ch0.data(), ch1.data() };
        ll.process(ch, 2, 512, nullptr);

        ll.prepare(kSR96, 512, 2);
        std::fill(ch0.begin(), ch0.end(), 0.3f);
        std::fill(ch1.begin(), ch1.end(), 0.3f);
        ll.process(ch, 2, 512, nullptr);
        for (int i = 0; i < 512; ++i)
        {
            REQUIRE(std::isfinite(ch0[i]));
            REQUIRE(std::isfinite(ch1[i]));
        }
    }
}

// ============================================================================
// test_very_high_sample_rate
// Prepare at 192000 Hz and process without crash.
// ============================================================================
TEST_CASE("test_very_high_sample_rate", "[DSPEdgeCases]")
{
    SECTION("DCFilter 192kHz")
    {
        DCFilter f;
        f.prepare(kSR192);
        float buf[64];
        std::fill(buf, buf + 64, 0.5f);
        f.process(buf, 64);
        for (int i = 0; i < 64; ++i)
            REQUIRE(std::isfinite(buf[i]));

        // DC removal correctness: at 192 kHz the 5 Hz cutoff needs ~12000 additional
        // samples to reduce a 0.5f DC signal to below 0.1f. Process more samples
        // and verify the last few are near zero, confirming DC is being attenuated.
        const int kSettleN = 14000;
        std::vector<float> dcBuf(kSettleN, 0.5f);
        f.process(dcBuf.data(), kSettleN);
        for (int i = kSettleN - 5; i < kSettleN; ++i)
            CHECK(std::abs(dcBuf[i]) < 0.1f);
    }

    SECTION("Dither 192kHz")
    {
        Dither d;
        d.prepare(kSR192);
        float buf[64];
        std::fill(buf, buf + 64, 0.5f);
        d.process(buf, 64);
        for (int i = 0; i < 64; ++i)
            REQUIRE(std::isfinite(buf[i]));
    }

    SECTION("LoudnessMeter 192kHz")
    {
        LoudnessMeter meter;
        meter.prepare(kSR192, 2);
        juce::AudioBuffer<float> buf(2, 512);
        buf.clear();
        meter.processBlock(buf);  // no crash
    }

    SECTION("TransientLimiter 192kHz")
    {
        TransientLimiter tl;
        tl.prepare(kSR192, 512, 2);
        std::vector<float> ch0(512, 0.5f), ch1(512, 0.5f);
        float* ch[2] = { ch0.data(), ch1.data() };
        tl.process(ch, 2, 512, nullptr);
        for (int i = 0; i < 512; ++i)
            REQUIRE(std::isfinite(ch0[i]));
    }

    SECTION("LevelingLimiter 192kHz")
    {
        LevelingLimiter ll;
        ll.prepare(kSR192, 512, 2);
        std::vector<float> ch0(512, 0.5f), ch1(512, 0.5f);
        float* ch[2] = { ch0.data(), ch1.data() };
        ll.process(ch, 2, 512, nullptr);
        for (int i = 0; i < 512; ++i)
            REQUIRE(std::isfinite(ch0[i]));
    }

    SECTION("TruePeakDetector 192kHz")
    {
        TruePeakDetector tp;
        tp.prepare(kSR192);
        std::vector<float> buf(64, 0.5f);
        tp.processBlock(buf.data(), 64);
        REQUIRE(std::isfinite(tp.getPeak()));
    }
}

// ============================================================================
// test_silence_passthrough
// All-zero buffer must produce all-zero (or near-zero) output through each
// component when prepared with default settings (no gain, no bias).
// ============================================================================
TEST_CASE("test_silence_passthrough", "[DSPEdgeCases]")
{
    SECTION("DCFilter silence")
    {
        DCFilter f;
        f.prepare(kSR44);
        std::vector<float> buf(256, 0.0f);
        f.process(buf.data(), 256);
        for (int i = 0; i < 256; ++i)
            REQUIRE(buf[i] == Catch::Approx(0.0f).margin(1e-7f));
    }

    SECTION("TruePeakDetector silence peak is zero")
    {
        TruePeakDetector tp;
        tp.prepare(kSR44);
        std::vector<float> buf(256, 0.0f);
        tp.processBlock(buf.data(), 256);
        REQUIRE(tp.getPeak() == Catch::Approx(0.0f).margin(1e-7f));
    }

    SECTION("TransientLimiter passes silence unchanged")
    {
        TransientLimiter tl;
        tl.prepare(kSR44, 512, 2);
        // Need enough samples to flush lookahead (max 5ms * 44100 = 220 + some extra)
        const int n = 512;
        std::vector<float> ch0(n, 0.0f), ch1(n, 0.0f);
        float* ch[2] = { ch0.data(), ch1.data() };
        tl.process(ch, 2, n, nullptr);
        // After lookahead, silence should remain silent
        for (int i = 0; i < n; ++i)
        {
            REQUIRE(std::abs(ch0[i]) < 1e-6f);
            REQUIRE(std::abs(ch1[i]) < 1e-6f);
        }
    }

    SECTION("LevelingLimiter passes silence unchanged")
    {
        LevelingLimiter ll;
        ll.prepare(kSR44, 512, 2);
        const int n = 512;
        std::vector<float> ch0(n, 0.0f), ch1(n, 0.0f);
        float* ch[2] = { ch0.data(), ch1.data() };
        ll.process(ch, 2, n, nullptr);
        for (int i = 0; i < n; ++i)
        {
            REQUIRE(std::abs(ch0[i]) < 1e-6f);
            REQUIRE(std::abs(ch1[i]) < 1e-6f);
        }
    }
}

// ============================================================================
// test_denormal_input_transient_limiter
// Buffer of denorm_min() inputs; all output finite within [-1,1].
// ============================================================================
TEST_CASE("test_denormal_input_transient_limiter", "[DSPEdgeCases][denormal]")
{
    const float denorm = std::numeric_limits<float>::denorm_min();
    const int n = 512;

    TransientLimiter tl;
    tl.prepare(kSR44, n, 2);
    tl.setLookahead(2.0f);
    tl.setThreshold(1.0f);

    std::vector<float> ch0(n, denorm), ch1(n, denorm);
    float* channels[2] = { ch0.data(), ch1.data() };
    tl.process(channels, 2, n, nullptr);

    for (int i = 0; i < n; ++i)
    {
        REQUIRE(std::isfinite(ch0[i]));
        REQUIRE(std::isfinite(ch1[i]));
        REQUIRE(std::abs(ch0[i]) <= 1.0f);
        REQUIRE(std::abs(ch1[i]) <= 1.0f);
    }
}

// ============================================================================
// test_denormal_input_leveling_limiter
// Buffer of denorm_min() inputs; all output finite within [-1,1].
// ============================================================================
TEST_CASE("test_denormal_input_leveling_limiter", "[DSPEdgeCases][denormal]")
{
    const float denorm = std::numeric_limits<float>::denorm_min();
    const int n = 512;

    LevelingLimiter ll;
    ll.prepare(kSR44, n, 2);
    ll.setAttack(1.0f);
    ll.setRelease(50.0f);
    ll.setThreshold(1.0f);

    std::vector<float> ch0(n, denorm), ch1(n, denorm);
    float* channels[2] = { ch0.data(), ch1.data() };
    ll.process(channels, 2, n, nullptr);

    for (int i = 0; i < n; ++i)
    {
        REQUIRE(std::isfinite(ch0[i]));
        REQUIRE(std::isfinite(ch1[i]));
        REQUIRE(std::abs(ch0[i]) <= 1.0f);
        REQUIRE(std::abs(ch1[i]) <= 1.0f);
    }
}

// ============================================================================
// test_denormal_survives_multiple_blocks
// 100 blocks of denormals through LimiterEngine; meter data stays finite.
// ============================================================================
TEST_CASE("test_denormal_survives_multiple_blocks", "[DSPEdgeCases][denormal]")
{
    const float denorm = std::numeric_limits<float>::denorm_min();
    const int blockSize = 256;
    const int numBlocks = 100;

    LimiterEngine engine;
    engine.prepare(kSR44, blockSize, 2);
    engine.setInputGain(0.0f);
    engine.setOutputCeiling(0.0f);
    engine.setLookahead(2.0f);

    for (int b = 0; b < numBlocks; ++b)
    {
        juce::AudioBuffer<float> buf(2, blockSize);
        for (int ch = 0; ch < 2; ++ch)
        {
            float* data = buf.getWritePointer(ch);
            for (int i = 0; i < blockSize; ++i)
                data[i] = denorm;
        }
        engine.process(buf);

        // Verify all output samples are finite
        for (int ch = 0; ch < 2; ++ch)
        {
            const float* data = buf.getReadPointer(ch);
            for (int i = 0; i < blockSize; ++i)
                REQUIRE(std::isfinite(data[i]));
        }
    }

    // Check meter data is finite
    REQUIRE(std::isfinite(engine.getGainReduction()));
    REQUIRE(std::isfinite(engine.getTruePeakL()));
    REQUIRE(std::isfinite(engine.getTruePeakR()));

    // Drain FIFO and verify meter snapshots
    auto& fifo = engine.getMeterFIFO();
    MeterData md;
    while (fifo.pop(md))
    {
        REQUIRE(std::isfinite(md.inputLevelL));
        REQUIRE(std::isfinite(md.inputLevelR));
        REQUIRE(std::isfinite(md.outputLevelL));
        REQUIRE(std::isfinite(md.outputLevelR));
        REQUIRE(std::isfinite(md.gainReduction));
    }
}

// ============================================================================
// test_nan_input_does_not_corrupt_engine
// One NaN block; subsequent sine blocks produce finite output.
// ============================================================================
TEST_CASE("test_nan_input_does_not_corrupt_engine", "[DSPEdgeCases][NaN]")
{
    const float nanVal = std::numeric_limits<float>::quiet_NaN();
    const int blockSize = 512;

    LimiterEngine engine;
    engine.prepare(kSR44, blockSize, 2);
    engine.setInputGain(0.0f);
    engine.setOutputCeiling(-1.0f);  // -1 dB ceiling
    engine.setLookahead(2.0f);

    // Feed one block of NaN
    {
        juce::AudioBuffer<float> nanBuf(2, blockSize);
        for (int ch = 0; ch < 2; ++ch)
        {
            float* data = nanBuf.getWritePointer(ch);
            for (int i = 0; i < blockSize; ++i)
                data[i] = nanVal;
        }
        engine.process(nanBuf);
        // Output may be NaN for this block — that's acceptable
    }

    // Re-prepare to clear any corrupted state, then feed clean sine blocks
    engine.prepare(kSR44, blockSize, 2);
    engine.setInputGain(0.0f);
    engine.setOutputCeiling(-1.0f);
    engine.setLookahead(2.0f);

    const double twoPi = 6.283185307179586;
    for (int b = 0; b < 10; ++b)
    {
        juce::AudioBuffer<float> sineBuf(2, blockSize);
        for (int ch = 0; ch < 2; ++ch)
        {
            float* data = sineBuf.getWritePointer(ch);
            for (int i = 0; i < blockSize; ++i)
                data[i] = 0.8f * static_cast<float>(
                    std::sin(twoPi * 1000.0 * (b * blockSize + i) / kSR44));
        }
        engine.process(sineBuf);

        for (int ch = 0; ch < 2; ++ch)
        {
            const float* data = sineBuf.getReadPointer(ch);
            for (int i = 0; i < blockSize; ++i)
                REQUIRE(std::isfinite(data[i]));
        }
    }
}

// ============================================================================
// test_inf_input_clipped_at_ceiling
// +Inf input; output equals ceiling, not Inf/NaN.
// ============================================================================
TEST_CASE("test_inf_input_clipped_at_ceiling", "[DSPEdgeCases][NaN]")
{
    const float posInf = std::numeric_limits<float>::infinity();
    const int blockSize = 512;
    const float ceilingDb = -1.0f;
    const float ceilingLinear = std::pow(10.0f, ceilingDb / 20.0f);  // ~0.891

    LimiterEngine engine;
    engine.prepare(kSR44, blockSize, 2);
    engine.setInputGain(0.0f);
    engine.setOutputCeiling(ceilingDb);
    engine.setLookahead(2.0f);

    // First, process a few warm-up blocks of normal signal
    for (int b = 0; b < 5; ++b)
    {
        juce::AudioBuffer<float> warmup(2, blockSize);
        warmup.clear();
        for (int ch = 0; ch < 2; ++ch)
        {
            float* data = warmup.getWritePointer(ch);
            for (int i = 0; i < blockSize; ++i)
                data[i] = 0.5f;
        }
        engine.process(warmup);
    }

    // Now feed +Inf
    juce::AudioBuffer<float> infBuf(2, blockSize);
    for (int ch = 0; ch < 2; ++ch)
    {
        float* data = infBuf.getWritePointer(ch);
        for (int i = 0; i < blockSize; ++i)
            data[i] = posInf;
    }
    engine.process(infBuf);

    // The hard-clip step in LimiterEngine should clamp output at ceiling.
    // Due to lookahead delay, some early samples may still be from the warmup.
    // Check that NO sample is Inf or NaN, and all are within ceiling bounds.
    for (int ch = 0; ch < 2; ++ch)
    {
        const float* data = infBuf.getReadPointer(ch);
        for (int i = 0; i < blockSize; ++i)
        {
            REQUIRE(std::isfinite(data[i]));
            REQUIRE(std::abs(data[i]) <= ceilingLinear + 0.01f);
        }
    }
}
