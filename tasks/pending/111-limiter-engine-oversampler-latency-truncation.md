# Task 111: LimiterEngine — Oversampler Latency Truncated Instead of Rounded, Under-Reporting Host Latency

## Description

`LimiterEngine::getLatencySamples()` reports the total plugin latency to the host for DAW track alignment:

```cpp
int LimiterEngine::getLatencySamples() const
{
    const float lookaheadMs = mLookaheadMs.load();
    const int lookaheadSamples = static_cast<int>(lookaheadMs * 0.001 * mSampleRate);
    const int oversamplerLatency = static_cast<int>(mOversampler.getLatencySamples());  // line 481
    return lookaheadSamples + oversamplerLatency;
}
```

`mOversampler.getLatencySamples()` wraps JUCE's `Oversampling::getLatencyInSamples()`, which returns a **`float`**. JUCE accumulates sub-sample-precise group delays across filter stages and returns them as a float. For the IIR polyphase filter mode (`filterHalfBandPolyphaseIIR`), the returned value can be a non-integer (e.g., `0.5`, `1.5`, or similar).

Using `static_cast<int>` **truncates** toward zero, potentially under-reporting the latency by up to 1 sample. For example:
- Actual oversampler latency: 0.7 samples → reported: 0 samples (−1 sample error)
- Actual oversampler latency: 1.5 samples → reported: 1 sample (−1 sample error)

A 1-sample under-report causes the DAW to apply 1 sample too little delay compensation to the dry signal, creating a 1-sample offset between the processed and dry tracks — audible as a very subtle comb filter when blended in a mix context.

**Fix:** Replace the truncating cast with `std::lround()` (or `static_cast<int>(std::round(...))`):

```cpp
const int oversamplerLatency = static_cast<int>(std::lround(mOversampler.getLatencySamples()));
```

Apply the same fix to the lookahead calculation to be consistent:

```cpp
const int lookaheadSamples = static_cast<int>(std::round(lookaheadMs * 0.001 * mSampleRate));
```

## Produces
None

## Consumes
None

## Relevant Files
Modify: `M-LIM/src/dsp/LimiterEngine.cpp` — fix `getLatencySamples()`, line 479–482
Read: `M-LIM/src/dsp/LimiterEngine.h` — `getLatencySamples()` declaration
Read: `M-LIM/tests/dsp/test_limiter_engine.cpp` — existing tests for latency

## Acceptance Criteria
- [ ] Run: `cd /workspace/M-LIM/build && ctest -R LimiterEngine --output-on-failure` → Expected: all tests pass, exit 0
- [ ] Run: `grep -n "static_cast<int>(mOversampler" /workspace/M-LIM/src/dsp/LimiterEngine.cpp` → Expected: no output (raw truncating cast removed)
- [ ] Run: `cd /workspace/M-LIM/build && ctest --output-on-failure` → Expected: all tests pass, exit 0

## Tests
- Unit: `tests/dsp/test_limiter_engine.cpp::test_latency_reported_rounds_not_truncates` — mock a scenario (or parametrise) where the oversampler reports a fractional latency of 0.7 samples; verify `getLatencySamples()` returns 1, not 0. This can be tested by checking that the latency is consistent with `std::round(oversamplerLatency + lookaheadSamples)` for various lookahead values.
- Unit: `tests/dsp/test_limiter_engine.cpp::test_latency_nonzero_at_2x_oversampling` — prepare at 2× oversampling, assert `getLatencySamples() > 0`

## Technical Details

Change in `M-LIM/src/dsp/LimiterEngine.cpp`, function `getLatencySamples()`:

```cpp
// Before:
const int lookaheadSamples    = static_cast<int>(lookaheadMs * 0.001 * mSampleRate);
const int oversamplerLatency  = static_cast<int>(mOversampler.getLatencySamples());

// After:
const int lookaheadSamples    = static_cast<int>(std::round(lookaheadMs * 0.001 * mSampleRate));
const int oversamplerLatency  = static_cast<int>(std::lround(mOversampler.getLatencySamples()));
```

`std::lround` requires `<cmath>` (already included). On most platforms `lround` is faster than `round` + cast because it avoids the intermediate float.

## Dependencies
None
