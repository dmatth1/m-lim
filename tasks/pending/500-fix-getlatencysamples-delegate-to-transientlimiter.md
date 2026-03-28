# Task 500: Fix LimiterEngine::getLatencySamples() Rounding Inconsistency

## Description
`LimiterEngine::getLatencySamples()` independently recomputes lookahead latency from the raw
millisecond parameter using truncation:

```cpp
const int lookaheadSamples = static_cast<int>(lookaheadMs * 0.001 * mSampleRate);
```

However, `TransientLimiter::getLatencyInSamples()` computes this via a truncate-then-round
chain at the oversampled rate, so the two values diverge by ±1 sample.

**Concrete example** (44100 Hz, 4× OS, lookahead 2.6 ms):
- `TransientLimiter::setLookahead()`: `mLookaheadSamples = int(2.6 * 0.001 * 176400) = 458`
- `TransientLimiter::getLatencyInSamples()`: `round(458 / 4.0) = round(114.5) = 115`
- `LimiterEngine::getLatencySamples()` lookahead part: `int(2.6 * 0.001 * 44100) = 114`

Result: host is told 114 samples but actual audio delay is 115 → DAW delay compensation
is off by 1 sample → audible phase artifact when blending the limited track with a dry
reference.

**Fix**: Replace the independent lookahead formula in `getLatencySamples()` with a delegation
to `mTransientLimiter.getLatencyInSamples()`:

```cpp
int LimiterEngine::getLatencySamples() const
{
    const int lookaheadSamples   = mTransientLimiter.getLatencyInSamples();
    const int oversamplerLatency = static_cast<int>(std::lround(mOversampler.getLatencySamples()));
    return lookaheadSamples + oversamplerLatency;
}
```

## Produces
None

## Consumes
None

## Relevant Files
Modify: `src/dsp/LimiterEngine.cpp` — rewrite `getLatencySamples()` to call
  `mTransientLimiter.getLatencyInSamples()` instead of recalculating from ms
Read: `src/dsp/TransientLimiter.h` — `getLatencyInSamples()` API contract
Read: `src/dsp/TransientLimiter.cpp` — `setLookahead()` and `getLatencyInSamples()` implementation
Read: `src/dsp/LimiterEngine.h` — `getLatencySamples()` declaration
Read: `src/PluginProcessor.cpp` — `updateLatency()` / `setLatencySamples()` call sites

## Acceptance Criteria
- [ ] Run: `grep -n "lookaheadMs.*mSampleRate" src/dsp/LimiterEngine.cpp` → Expected: the truncation formula does NOT appear inside `getLatencySamples()`
- [ ] Run: `cmake --build build --target MLIM_Standalone_Standalone -j$(nproc) 2>&1 | tail -3` → Expected: build succeeds, exit 0
- [ ] Run: `cd build && ctest -R MLIMTests --output-on-failure 2>&1 | tail -10` → Expected: all tests pass

## Tests
- Unit: `tests/dsp/test_limiter_engine.cpp::test_getlatencysamples_matches_transientlimiter` — prepare engine at 44100 Hz, 4× OS, lookahead = 2.6 ms; verify `getLatencySamples()` equals `transientLimiter.getLatencyInSamples() + oversampler latency`
- Unit: `tests/dsp/test_limiter_engine.cpp::test_getlatencysamples_all_os_factors` — for OS factors 1x–4x and lookahead values (0.5, 1.0, 2.6, 5.0 ms), verify that `getLatencySamples()` matches the actual impulse delay measured in `process()` output within ±1 sample
- Unit: `tests/dsp/test_limiter_engine.cpp::test_getlatencysamples_0p1ms_4x_48k` — verify that at 0.1 ms lookahead, 4× OS, 48 kHz, `getLatencySamples()` returns 5 (rounds up) not 4 (truncates)

## Technical Details
- `mTransientLimiter` is `private` in `LimiterEngine`; `getLatencySamples()` is a member so it can call `mTransientLimiter.getLatencyInSamples()` directly
- Before `prepare()` is called, `getLatencyInSamples()` returns 0 and `mOversampler.getLatencySamples()` returns 0 — same result as today
- `LimiterEngine::getLatencySamples()` is the sole input to `setLatencySamples()` in `PluginProcessor::updateLatency()`

## Dependencies
None
