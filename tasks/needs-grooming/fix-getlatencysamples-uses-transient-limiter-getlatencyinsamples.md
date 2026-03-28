# Task: Fix getLatencySamples() latency off-by-1 when oversampling active

## Description
`LimiterEngine::getLatencySamples()` recalculates the lookahead latency independently of
`TransientLimiter::getLatencyInSamples()`, using **truncation** while the TransientLimiter
uses `std::round`. This causes a ±1 sample mismatch in the latency reported to the host
for specific combinations of lookahead, sample rate, and oversampling factor.

**Concrete example** (root of the bug):

- 44100 Hz, 4× oversampling (176400 Hz operating rate), lookahead = 2.6 ms:

  `TransientLimiter::setLookahead()` (line 66):
  ```cpp
  mLookaheadSamples = int(2.6 * 0.001 * 176400) = int(458.64) = 458   // truncated
  ```
  `TransientLimiter::getLatencyInSamples()` — the **actual** delay applied to audio:
  ```cpp
  int(round(458.0 / 4.0)) = int(round(114.5)) = 115   // rounds up
  ```
  `LimiterEngine::getLatencySamples()` — what gets reported to the host:
  ```cpp
  int(2.6 * 0.001 * 44100) = int(114.66) = 114          // truncated → WRONG
  ```

  Result: host reports 114 samples but actual audio delay is 115 samples → DAW
  delay compensation is off by 1 sample → audible phase artifact when blending
  the limited track with a dry reference.

**Fix**: Replace the independent lookahead recalculation in `getLatencySamples()` with a
call to `mTransientLimiter.getLatencyInSamples()`, which already accounts for the
OS-domain truncation + round-trip:

```cpp
int LimiterEngine::getLatencySamples() const
{
    const int lookaheadSamples    = mTransientLimiter.getLatencyInSamples();
    const int oversamplerLatency  = static_cast<int>(std::lround(mOversampler.getLatencySamples()));
    return lookaheadSamples + oversamplerLatency;
}
```

`TransientLimiter::getLatencyInSamples()` is the single source of truth for actual
lookahead delay; removing the duplicate calculation ensures the reported latency matches
reality.

## Produces
None

## Consumes
None

## Relevant Files
Modify: `src/dsp/LimiterEngine.cpp` — rewrite `getLatencySamples()` to call
  `mTransientLimiter.getLatencyInSamples()` instead of recalculating from ms
Read: `src/dsp/TransientLimiter.h` — `getLatencyInSamples()` API contract
Read: `src/dsp/TransientLimiter.cpp` — implementation of `setLookahead()` and
  `getLatencyInSamples()` to understand the truncation→round chain
Read: `src/dsp/LimiterEngine.h` — `getLatencySamples()` declaration
Read: `src/PluginProcessor.cpp` — `updateLatency()` / `setLatencySamples()` call sites

## Acceptance Criteria
- [ ] Run: `cd build && ctest -R MLIMTests --output-on-failure 2>&1 | tail -10` → Expected: all tests pass
- [ ] Run: `cmake --build build --target MLIM_Standalone_Standalone -j$(nproc) 2>&1 | tail -3` → Expected: build succeeds
- [ ] Run: `grep -n "getLatencyInSamples\|mLookaheadMs.*mSampleRate" M-LIM/src/dsp/LimiterEngine.cpp` → Expected: the lookahead-from-ms pattern `lookaheadMs * 0.001 * mSampleRate` does NOT appear in `getLatencySamples()`; only `mTransientLimiter.getLatencyInSamples()` is used

## Tests
- Unit: `tests/dsp/test_limiter_engine.cpp::test_getlatencysamples_matches_transientlimiter` — prepare engine at 44100 Hz, 4× OS, lookahead = 2.6 ms; verify `getLatencySamples()` equals `mTransientLimiter.getLatencyInSamples()` + oversampler latency (test must expose TransientLimiter latency via a method or friend accessor)
- Unit: `tests/dsp/test_limiter_engine.cpp::test_getlatencysamples_all_os_factors` — for OS factors 0–5 and representative lookahead values (0.5, 1.0, 2.6, 5.0 ms), verify that the latency reported by `getLatencySamples()` matches the actual impulse delay measured in `process()` output (within ±1 sample to account for filter ramp-up)

## Technical Details
- `mTransientLimiter` is `private` in `LimiterEngine`; `getLatencySamples()` is a member function so it can call `mTransientLimiter.getLatencyInSamples()` directly
- `TransientLimiter::getLatencyInSamples()` is declared `const` and uses `mSampleRate` / `mOriginalSampleRate` set during `prepare()` — it returns 0 before `prepare()` is called (both default to 44100, osFactor = 1, and `mLookaheadSamples = 0`)
- `LimiterEngine::getLatencySamples()` is also called before `prepare()` (from `updateLatency()` in `setStateInformation`). Before `prepare()`, `TransientLimiter::getLatencyInSamples()` returns 0 and `mOversampler.getLatencySamples()` returns 0, so the result remains 0 — same as today

## Dependencies
None
