# Task 162: Fix 1-Sample Latency Discrepancy Between TransientLimiter and getLatencySamples()

## Description
`LimiterEngine::getLatencySamples()` and `TransientLimiter::setLookahead()` compute the
lookahead delay in samples using inconsistent rounding, causing a 1-sample DAW alignment
error at certain lookahead values.

**Locations**:
- `M-LIM/src/dsp/LimiterEngine.cpp` line ~475 (`getLatencySamples()`)
- `M-LIM/src/dsp/TransientLimiter.cpp` line ~58 (`setLookahead()`)

**Bug**:
```cpp
// TransientLimiter::setLookahead — TRUNCATES (int cast):
mLookaheadSamples = static_cast<int>(clamped * 0.001f * static_cast<float>(mSampleRate));

// LimiterEngine::getLatencySamples — ROUNDS:
const int lookaheadSamples = static_cast<int>(std::round(lookaheadMs * 0.001 * mSampleRate));
```

**Example at 44100 Hz, lookahead = 5 ms**:
- Actual delay in TransientLimiter: `int(5 * 0.001 * 44100)` = `int(220.5)` = **220 samples**
- Reported to host: `round(5 * 0.001 * 44100)` = `round(220.5)` = **221 samples**

The host compensates 221 samples but the actual processing delay is only 220. This means
in a zero-latency monitoring or delay-compensated routing context, the plugin's output is
1 sample early relative to the delay-compensated path. Audible at low latency (e.g., live
monitoring, stem mixing).

There is a compounding issue: `TransientLimiter` is prepared at the **upsampled** sample
rate. `setLookahead(ms)` is called with the engine's `mLookaheadMs` after prepare, so
the conversion is `int(ms * 0.001 * usSampleRate)`. The engine reports latency using the
original sample rate `mSampleRate`, not the upsampled rate. This means:
- At 4x OS (176400 Hz), actual delay = `int(5 * 0.001 * 176400)` = 882 upsampled samples = 220.5 original samples
- Reported = `round(5 * 0.001 * 44100)` = 221 original samples

Both conversions should agree. The fix is to use the same rounding (truncation via `int` cast,
which is what `setLookahead` uses) in `getLatencySamples()`.

**Fix**: Change `getLatencySamples()` to use `static_cast<int>` (truncation) instead of
`std::round`, matching `TransientLimiter::setLookahead()`.

```cpp
// LimiterEngine.cpp — getLatencySamples()
// Before:
const int lookaheadSamples = static_cast<int>(std::round(lookaheadMs * 0.001 * mSampleRate));

// After:
const int lookaheadSamples = static_cast<int>(lookaheadMs * 0.001 * mSampleRate);
```

## Produces
None

## Consumes
None

## Relevant Files
Modify: `M-LIM/src/dsp/LimiterEngine.cpp` — fix rounding in `getLatencySamples()` (~line 475)
Read:   `M-LIM/src/dsp/TransientLimiter.cpp` — verify `setLookahead()` uses truncation (~line 58)
Read:   `M-LIM/src/PluginProcessor.cpp` — verify `updateLatency()` calls `getLatencySamples()`

## Acceptance Criteria
- [ ] Run: `grep -n "std::round\|lround" M-LIM/src/dsp/LimiterEngine.cpp` → Expected: no match in `getLatencySamples()` function (the `std::lround` for the oversampler float is OK to keep)
- [ ] Run: `cd build && cmake --build . -j$(nproc) 2>&1 | tail -5` → Expected: no errors
- [ ] Run: `cd build && ctest --output-on-failure 2>&1 | tail -10` → Expected: all tests pass

## Tests
- Unit: write a test at 44100 Hz with 5 ms lookahead (no oversampling) asserting that
  `getLatencySamples()` returns 220 (truncation, matching `setLookahead`) not 221 (rounding)
- Unit: write a test at 48000 Hz with 1 ms lookahead asserting `getLatencySamples()` returns 48
- Unit: write a test with 4x oversampling at 44100 Hz with 5 ms lookahead verifying the reported
  latency equals `int(5 * 0.001 * 44100)` + oversampler latency

## Technical Details
The `std::lround` on line ~476 (for the oversampler latency, which returns a `float`) is
unrelated and correct to keep. Only the lookahead conversion needs fixing.

## Dependencies
None
