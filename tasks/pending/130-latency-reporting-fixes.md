# Task 130: Latency Reporting Fixes — Round Oversampler Latency and Fix getTailLengthSeconds

## Description
Two related latency reporting bugs in the plugin.

**Issue 1 — Oversampler latency truncated instead of rounded (LimiterEngine.cpp):**
`LimiterEngine::getLatencySamples()` casts `mOversampler.getLatencySamples()` (a `float`) to `int` with `static_cast<int>`, which truncates toward zero. JUCE's `Oversampling::getLatencyInSamples()` returns a float that can be non-integer (e.g. 0.7, 1.5). Truncation under-reports latency by up to 1 sample, causing DAW track misalignment when blending dry/wet signals.

Fix in `LimiterEngine::getLatencySamples()`:
```cpp
// Before:
const int lookaheadSamples   = static_cast<int>(lookaheadMs * 0.001 * mSampleRate);
const int oversamplerLatency = static_cast<int>(mOversampler.getLatencySamples());
// After:
const int lookaheadSamples   = static_cast<int>(std::round(lookaheadMs * 0.001 * mSampleRate));
const int oversamplerLatency = static_cast<int>(std::lround(mOversampler.getLatencySamples()));
```

**Issue 2 — getTailLengthSeconds() omits oversampler latency (PluginProcessor.cpp):**
`MLIMAudioProcessor::getTailLengthSeconds()` returns only the lookahead component, ignoring the oversampler's filter ringout (already included in `getLatencySamples()`). This causes offline export tails to be cut short when oversampling is enabled.

Fix:
```cpp
double MLIMAudioProcessor::getTailLengthSeconds() const
{
    const double sr = getSampleRate();
    if (sr > 0.0)
        return static_cast<double>(getLatencySamples()) / sr;
    // Fallback before prepareToPlay (sr == 0): use lookahead only
    if (pLookahead != nullptr)
        return static_cast<double>(pLookahead->load()) * 0.001;
    return 0.0;
}
```

## Produces
None

## Consumes
None

## Relevant Files
Modify: `M-LIM/src/dsp/LimiterEngine.cpp` — fix `getLatencySamples()` truncating casts (~line 479–482)
Modify: `M-LIM/src/PluginProcessor.cpp` — fix `getTailLengthSeconds()` (~lines 38–43)
Read:   `M-LIM/src/dsp/LimiterEngine.h`  — `getLatencySamples()` declaration
Read:   `M-LIM/src/PluginProcessor.h`    — `updateLatency()` helper

## Acceptance Criteria
- [ ] Run: `grep -n "static_cast<int>(mOversampler" M-LIM/src/dsp/LimiterEngine.cpp` → Expected: no output (truncating cast removed)
- [ ] Run: `grep -A 8 "getTailLengthSeconds" M-LIM/src/PluginProcessor.cpp` → Expected: implementation references `getLatencySamples()` or `getSampleRate()`
- [ ] Run: `cmake --build build -j$(nproc) 2>&1 | tail -5` → Expected: zero errors
- [ ] Run: `cd build && ctest --output-on-failure 2>&1 | tail -10` → Expected: all tests pass

## Tests
- Unit: `tests/dsp/test_limiter_engine.cpp::test_latency_reported_rounds_not_truncates` — prepare at 2× oversampling; assert `getLatencySamples() > 0` and equals `std::lround(oversamplerFloat + lookaheadSamples)`.
- Integration: `tests/integration/test_plugin_processor.cpp::test_tail_length_includes_oversampler` — after `prepareToPlay(44100, 512)` with 2× oversampling: `getTailLengthSeconds() >= getLatencySamples() / 44100.0 - 0.001`.

## Technical Details
`std::lround` requires `<cmath>` (already included in LimiterEngine.cpp). `getSampleRate()` returns 0.0 before `prepareToPlay`; the fallback pLookahead path handles that case. Both fixes are small (2–4 line changes).

## Dependencies
None
