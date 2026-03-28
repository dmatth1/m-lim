# Fix LimiterEngine::getLatencySamples() Rounding Inconsistency

## Description
`LimiterEngine::getLatencySamples()` (LimiterEngine.cpp:539-546) independently recomputes
lookahead latency from the raw millisecond parameter using truncation:

```cpp
const int lookaheadSamples = static_cast<int>(lookaheadMs * 0.001 * mSampleRate);
```

However, `TransientLimiter::getLatencyInSamples()` (TransientLimiter.cpp:128-138) also
computes this using rounding:

```cpp
const double osFactor = mSampleRate / mOriginalSampleRate;
return static_cast<int>(std::round(static_cast<double>(mLookaheadSamples) / osFactor));
```

`mLookaheadSamples` is itself set by `TransientLimiter::setLookahead()` via truncation at the
**upsampled** rate: `mLookaheadSamples = floor(ms * 0.001 * upsampledRate)`.

When dividing back to the original rate, the rounding applied in `getLatencyInSamples()` may
produce a different integer than the truncation in `LimiterEngine::getLatencySamples()`.

**Concrete example** (0.1ms lookahead, 48 kHz, 4x oversampling = 192 kHz):
- `mLookaheadSamples = floor(0.1 * 0.001 * 192000) = floor(19.2) = 19`
- `TransientLimiter::getLatencyInSamples() = round(19 / 4) = round(4.75) = 5`
- `LimiterEngine::getLatencySamples()` lookahead part: `floor(0.1 * 0.001 * 48000) = floor(4.8) = 4`

The host is told **4** samples of latency but the actual physical delay through the lookahead
buffer is **4.75** original-rate samples (rounded to 5 by the TransientLimiter). This causes
the host's delay compensation to be off by 1 sample — resulting in 1-sample misalignment
between the plugin output and other delay-compensated tracks in the DAW.

**Fix**: Replace the independent lookahead formula in `LimiterEngine::getLatencySamples()` with
a delegation to `mTransientLimiter.getLatencyInSamples()`, which is the authoritative source
for the actual delay buffer depth:

```cpp
int LimiterEngine::getLatencySamples() const
{
    // Delegate to TransientLimiter which tracks the actual delay buffer depth.
    const int lookaheadSamples = mTransientLimiter.getLatencyInSamples();
    const int oversamplerLatency = static_cast<int>(std::lround(mOversampler.getLatencySamples()));
    return lookaheadSamples + oversamplerLatency;
}
```

## Produces
None

## Consumes
None

## Relevant Files
Modify: `src/dsp/LimiterEngine.cpp` — fix `getLatencySamples()` lines 539-546
Read: `src/dsp/TransientLimiter.cpp` — `getLatencyInSamples()` lines 128-138 for reference
Read: `src/dsp/TransientLimiter.h` — `getLatencyInSamples()` declaration (line 74)
Read: `src/PluginProcessor.cpp` — `updateLatency()` calls `limiterEngine.getLatencySamples()`

## Acceptance Criteria
- [ ] Run: `grep -n "getLatencySamples" src/dsp/LimiterEngine.cpp` → Expected: the lookahead portion uses `mTransientLimiter.getLatencyInSamples()` not an independent formula
- [ ] Run: `cd build && cmake --build . --target MLIM_Standalone_Standalone -j$(nproc) 2>&1 | tail -5` → Expected: build succeeds, exit 0
- [ ] Run: `cd build && ctest -R "latency" --output-on-failure` → Expected: all latency tests pass

## Tests
- Unit: `tests/dsp/test_limiter_engine.cpp` — verify that `getLatencySamples()` matches `transientLimiter.getLatencyInSamples() + oversampler latency` at several oversampling factors (1x, 2x, 4x) and lookahead values (0ms, 1ms, 5ms)
- Unit: `tests/dsp/test_limiter_engine.cpp` — verify that at 0.1ms lookahead with 4x oversampling at 48kHz, `getLatencySamples()` returns 5 (rounds up) not 4 (truncates)

## Technical Details
- `LimiterEngine::getLatencySamples()` is the sole source for `setLatencySamples()` in `PluginProcessor::updateLatency()` (PluginProcessor.cpp:309)
- The `mTransientLimiter` member is already accessible at the call site — no new dependencies needed
- The `getLatencyInSamples()` method on TransientLimiter returns original-rate samples by dividing by the OS factor, which is exactly what the host needs

## Dependencies
None
