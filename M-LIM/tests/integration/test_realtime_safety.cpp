/**
 * test_realtime_safety.cpp — DSP real-time safety audit tests (Task 083)
 *
 * Verifies that processBlock() and LimiterEngine::process() perform zero heap
 * allocations in steady state. Uses a thread-local allocation counter and a
 * global operator new override that increments it only when tracking is enabled.
 *
 * The global operator new override affects the entire MLIMTests binary. The
 * thread-local g_trackAllocs flag gates counting so Catch2 framework overhead
 * does not create false positives outside the measured region.
 *
 * IMPORTANT: This technique only works in test builds. Do NOT include the
 * operator new override in production code.
 */

#include "catch2/catch_amalgamated.hpp"

#include "PluginProcessor.h"
#include "Parameters.h"
#include "dsp/LimiterEngine.h"
#include "dsp/LimiterAlgorithm.h"
#include "dsp/Oversampler.h"

#include <juce_audio_basics/juce_audio_basics.h>
#include <juce_audio_processors/juce_audio_processors.h>
#include <atomic>
#include <thread>
#include <cstdlib>
#include <cstring>
#include <new>
#include <cmath>

// Shared allocation-tracking utilities (AllocGuard, g_allocCount, g_trackAllocs).
#include "alloc_tracking.h"

// ---------------------------------------------------------------------------
// Allocation tracking — thread-local counters (external linkage so other
// translation units can access them via alloc_tracking.h).
// ---------------------------------------------------------------------------

// Thread-local so allocations on other threads (if any) are not counted,
// and so Catch2's own bookkeeping does not trigger false failures.
thread_local int  g_allocCount  = 0;
thread_local bool g_trackAllocs = false;

void* operator new(std::size_t sz)
{
    if (g_trackAllocs)
        ++g_allocCount;
    void* p = std::malloc(sz);
    if (!p)
        throw std::bad_alloc();
    return p;
}

void* operator new[](std::size_t sz)
{
    if (g_trackAllocs)
        ++g_allocCount;
    void* p = std::malloc(sz);
    if (!p)
        throw std::bad_alloc();
    return p;
}

void  operator delete  (void* p) noexcept               { std::free(p); }
void  operator delete[](void* p) noexcept               { std::free(p); }
void  operator delete  (void* p, std::size_t) noexcept  { std::free(p); }
void  operator delete[](void* p, std::size_t) noexcept  { std::free(p); }

// ---------------------------------------------------------------------------
// Helpers
// ---------------------------------------------------------------------------

static constexpr double kSampleRate  = 44100.0;
static constexpr int    kBlockSize   = 512;
static constexpr int    kNumChannels = 2;

static void fillTone(juce::AudioBuffer<float>& buf, float amplitude = 0.5f) noexcept
{
    const double phaseStep = 2.0 * juce::MathConstants<double>::pi * 440.0 / kSampleRate;
    for (int ch = 0; ch < buf.getNumChannels(); ++ch)
    {
        float* data = buf.getWritePointer(ch);
        for (int i = 0; i < buf.getNumSamples(); ++i)
            data[i] = amplitude * static_cast<float>(std::sin(phaseStep * i));
    }
}

// ============================================================================
// test_processblock_no_heap_allocation
//
// After 10 warm-up blocks (to settle lookahead buffers, internal state, etc.),
// count heap allocations during processBlock() for 100 blocks. Each individual
// call must produce zero allocations.
// ============================================================================
TEST_CASE("test_processblock_no_heap_allocation", "[RealtimeSafety]")
{
    MLIMAudioProcessor proc;
    proc.prepareToPlay(kSampleRate, kBlockSize);

    juce::AudioBuffer<float> buffer(kNumChannels, kBlockSize);
    juce::MidiBuffer         midiBuffer;

    // Warm up — not measured
    for (int i = 0; i < 10; ++i)
    {
        fillTone(buffer);
        proc.processBlock(buffer, midiBuffer);
    }

    // Measured: each block must not allocate
    for (int block = 0; block < 100; ++block)
    {
        fillTone(buffer);

        int allocsThisBlock;
        {
            AllocGuard guard;
            proc.processBlock(buffer, midiBuffer);
            allocsThisBlock = guard.count();
        }

        REQUIRE(allocsThisBlock == 0);
    }
}

// ============================================================================
// test_limiterengine_process_no_alloc
//
// Direct call to LimiterEngine::process() in steady state must produce zero
// heap allocations. Warm-up blocks ensure all internal lookahead / filter
// state is fully primed before measuring.
// ============================================================================
TEST_CASE("test_limiterengine_process_no_alloc", "[RealtimeSafety]")
{
    LimiterEngine engine;
    engine.prepare(kSampleRate, kBlockSize, kNumChannels);

    juce::AudioBuffer<float> buffer(kNumChannels, kBlockSize);

    // Warm up
    for (int i = 0; i < 10; ++i)
    {
        fillTone(buffer);
        engine.process(buffer);
    }

    // Measured
    fillTone(buffer);

    int allocCount;
    {
        AllocGuard guard;
        engine.process(buffer);
        allocCount = guard.count();
    }

    REQUIRE(allocCount == 0);
}

// ============================================================================
// test_all_features_no_alloc
//
// Verifies zero heap allocations in processBlock() when all optional features
// are active simultaneously: TruePeak, 2x oversampling, DC filter, and Dither.
// Parameters are set before prepareToPlay() so no deferred reallocation occurs
// inside the measured region.
// ============================================================================
TEST_CASE("test_all_features_no_alloc", "[RealtimeSafety]")
{
    MLIMAudioProcessor proc;

    // Enable all optional features before prepareToPlay so that the
    // internal buffers are sized correctly when prepare() runs.
    // Use setValueNotifyingHost() outside the measured region.
    auto* paramTruePeak  = proc.apvts.getParameter(ParamID::truePeakEnabled);
    auto* paramDCFilter  = proc.apvts.getParameter(ParamID::dcFilterEnabled);
    auto* paramDither    = proc.apvts.getParameter(ParamID::ditherEnabled);
    auto* paramOversampl = proc.apvts.getParameter(ParamID::oversamplingFactor);

    REQUIRE(paramTruePeak  != nullptr);
    REQUIRE(paramDCFilter  != nullptr);
    REQUIRE(paramDither    != nullptr);
    REQUIRE(paramOversampl != nullptr);

    // factor=1 → 2x oversampling. oversamplingFactor is a choice parameter
    // with 6 values (0..5); normalised value for index 1 = 1/5 = 0.2.
    paramTruePeak ->setValueNotifyingHost(1.0f);
    paramDCFilter ->setValueNotifyingHost(1.0f);
    paramDither   ->setValueNotifyingHost(1.0f);
    paramOversampl->setValueNotifyingHost(0.2f);

    // prepareToPlay forces the deferred oversampling reallocation synchronously.
    proc.prepareToPlay(kSampleRate, kBlockSize);

    juce::AudioBuffer<float> buffer(kNumChannels, kBlockSize);
    juce::MidiBuffer         midiBuffer;

    // -6 dBFS amplitude to exercise limiting path
    const float amplitude = 0.5f;

    // Warm up — not measured
    for (int i = 0; i < 10; ++i)
    {
        fillTone(buffer, amplitude);
        proc.processBlock(buffer, midiBuffer);
    }

    // Measured: each block must produce zero allocations
    for (int block = 0; block < 100; ++block)
    {
        fillTone(buffer, amplitude);

        int allocsThisBlock;
        {
            AllocGuard guard;
            proc.processBlock(buffer, midiBuffer);
            allocsThisBlock = guard.count();
        }

        INFO("Block " << block << " allocated " << allocsThisBlock << " time(s)");
        REQUIRE(allocsThisBlock == 0);
    }
}

// ============================================================================
// test_oversampling_steady_state_no_alloc
//
// After prepare() and 5 warm-up upsample/downsample cycles, a single cycle
// must produce zero heap allocations. juce::dsp::Oversampling pre-allocates
// all filter state during initProcessing(); processSamplesUp/Down must not
// allocate on subsequent calls.
// ============================================================================
TEST_CASE("test_oversampling_steady_state_no_alloc", "[RealtimeSafety]")
{
    Oversampler oversampler;
    oversampler.setFactor(1);   // 2x oversampling
    oversampler.prepare(kSampleRate, kBlockSize, kNumChannels);

    juce::AudioBuffer<float> buffer(kNumChannels, kBlockSize);

    // Warm up
    for (int i = 0; i < 5; ++i)
    {
        fillTone(buffer);
        auto upBlock = oversampler.upsample(buffer);
        oversampler.downsample(buffer);
    }

    // Measured: one upsample + downsample cycle
    fillTone(buffer);

    int allocCount;
    {
        AllocGuard guard;
        auto upBlock = oversampler.upsample(buffer);
        oversampler.downsample(buffer);
        allocCount = guard.count();
    }

    REQUIRE(allocCount == 0);
}

// ============================================================================
// test_threshold_sweep_no_nan
// A background thread sweeps the output ceiling from -20 to 0 dBFS while the
// audio thread processes 200 blocks at 0 dBFS amplitude. No output sample may
// be NaN or Inf.
// ============================================================================
TEST_CASE("test_threshold_sweep_no_nan", "[RealtimeSafety]")
{
    LimiterEngine engine;
    engine.prepare(kSampleRate, kBlockSize, kNumChannels);

    juce::AudioBuffer<float> buffer(kNumChannels, kBlockSize);

    // Warm up
    for (int i = 0; i < 10; ++i)
    {
        fillTone(buffer, 0.9f);
        engine.process(buffer);
    }

    std::atomic<bool> done{false};

    // Background: sweep ceiling continuously
    std::thread sweeper([&]() {
        float dB = -20.0f;
        const float step = 0.1f;
        while (!done.load(std::memory_order_relaxed))
        {
            engine.setOutputCeiling(dB);
            dB += step;
            if (dB > 0.0f) dB = -20.0f;
        }
    });

    int nanCount = 0;
    for (int block = 0; block < 200; ++block)
    {
        fillTone(buffer, 0.9f);
        engine.process(buffer);

        for (int ch = 0; ch < buffer.getNumChannels(); ++ch)
        {
            const float* data = buffer.getReadPointer(ch);
            for (int i = 0; i < buffer.getNumSamples(); ++i)
            {
                if (!std::isfinite(data[i]))
                    ++nanCount;
            }
        }
    }

    done.store(true, std::memory_order_relaxed);
    sweeper.join();

    INFO("NaN/Inf samples during threshold sweep: " << nanCount);
    REQUIRE(nanCount == 0);
}

// ============================================================================
// test_algorithm_switch_no_overshoot
// A background thread cycles through all 8 algorithms while the audio thread
// processes 200 blocks of loud audio. No output sample may exceed 1.01f.
// ============================================================================
TEST_CASE("test_algorithm_switch_no_overshoot", "[RealtimeSafety]")
{
    LimiterEngine engine;
    engine.prepare(kSampleRate, kBlockSize, kNumChannels);
    engine.setOutputCeiling(0.0f);

    juce::AudioBuffer<float> buffer(kNumChannels, kBlockSize);

    // Warm up
    for (int i = 0; i < 10; ++i)
    {
        fillTone(buffer, 0.9f);
        engine.process(buffer);
    }

    std::atomic<bool> done{false};

    std::thread algoSwitcher([&]() {
        int idx = 0;
        const LimiterAlgorithm algos[] = {
            LimiterAlgorithm::Transparent, LimiterAlgorithm::Punchy,
            LimiterAlgorithm::Dynamic,     LimiterAlgorithm::Aggressive,
            LimiterAlgorithm::Allround,    LimiterAlgorithm::Bus,
            LimiterAlgorithm::Safe,        LimiterAlgorithm::Modern
        };
        while (!done.load(std::memory_order_relaxed))
        {
            engine.setAlgorithm(algos[idx % 8]);
            ++idx;
        }
    });

    int overshootCount = 0;
    for (int block = 0; block < 200; ++block)
    {
        fillTone(buffer, 0.9f);
        engine.process(buffer);

        for (int ch = 0; ch < buffer.getNumChannels(); ++ch)
        {
            const float* data = buffer.getReadPointer(ch);
            for (int i = 0; i < buffer.getNumSamples(); ++i)
            {
                if (!std::isfinite(data[i]) || std::abs(data[i]) > 1.01f)
                    ++overshootCount;
            }
        }
    }

    done.store(true, std::memory_order_relaxed);
    algoSwitcher.join();

    INFO("Overshoot samples during algorithm switching: " << overshootCount);
    REQUIRE(overshootCount == 0);
}

// ============================================================================
// test_bypass_toggle_no_torn_output
// A background thread alternates bypass on/off while the audio thread
// processes 200 blocks. All output samples must be finite.
// ============================================================================
TEST_CASE("test_bypass_toggle_no_torn_output", "[RealtimeSafety]")
{
    LimiterEngine engine;
    engine.prepare(kSampleRate, kBlockSize, kNumChannels);
    engine.setOutputCeiling(0.0f);

    juce::AudioBuffer<float> buffer(kNumChannels, kBlockSize);

    // Warm up
    for (int i = 0; i < 10; ++i)
    {
        fillTone(buffer, 0.5f);
        engine.process(buffer);
    }

    std::atomic<bool> done{false};

    std::thread bypassToggler([&]() {
        bool bypass = false;
        while (!done.load(std::memory_order_relaxed))
        {
            engine.setBypass(bypass);
            bypass = !bypass;
        }
        engine.setBypass(false);
    });

    int badCount = 0;
    for (int block = 0; block < 200; ++block)
    {
        fillTone(buffer, 0.5f);
        engine.process(buffer);

        for (int ch = 0; ch < buffer.getNumChannels(); ++ch)
        {
            const float* data = buffer.getReadPointer(ch);
            for (int i = 0; i < buffer.getNumSamples(); ++i)
            {
                if (!std::isfinite(data[i]))
                    ++badCount;
            }
        }
    }

    done.store(true, std::memory_order_relaxed);
    bypassToggler.join();

    INFO("Non-finite samples during bypass toggling: " << badCount);
    REQUIRE(badCount == 0);
}

// ============================================================================
// test_concurrent_param_changes_no_allocation
// A background thread hammers parameter setters while the audio thread
// processes 100 blocks under AllocGuard. The audio thread must not allocate.
// ============================================================================
TEST_CASE("test_concurrent_param_changes_no_allocation", "[RealtimeSafety]")
{
    LimiterEngine engine;
    engine.prepare(kSampleRate, kBlockSize, kNumChannels);
    engine.setOutputCeiling(0.0f);

    juce::AudioBuffer<float> buffer(kNumChannels, kBlockSize);

    // Warm up (outside measured region)
    for (int i = 0; i < 10; ++i)
    {
        fillTone(buffer, 0.5f);
        engine.process(buffer);
    }

    std::atomic<bool> done{false};

    // Background thread: hammer setters as fast as possible
    std::thread paramHammer([&]() {
        int iter = 0;
        while (!done.load(std::memory_order_relaxed))
        {
            engine.setOutputCeiling(static_cast<float>(-(iter % 20)));
            engine.setInputGain(static_cast<float>((iter % 12) - 6));
            engine.setAttack(0.1f + static_cast<float>(iter % 10) * 0.5f);
            engine.setRelease(10.0f + static_cast<float>(iter % 20) * 5.0f);
            engine.setBypass((iter % 4) == 0);
            ++iter;
        }
        engine.setBypass(false);
    });

    // Audio thread: process under AllocGuard — must not allocate
    int totalAllocs = 0;
    for (int block = 0; block < 100; ++block)
    {
        fillTone(buffer, 0.5f);
        AllocGuard guard;
        engine.process(buffer);
        totalAllocs += guard.count();
    }

    done.store(true, std::memory_order_relaxed);
    paramHammer.join();

    INFO("Total allocations during concurrent param changes: " << totalAllocs);
    REQUIRE(totalAllocs == 0);
}
