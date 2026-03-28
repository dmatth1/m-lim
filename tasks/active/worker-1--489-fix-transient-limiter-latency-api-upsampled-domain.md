# Task 489: Fix TransientLimiter Latency API Returns Upsampled-Domain Value

## Description

`TransientLimiter::getLatencyInSamples()` (`TransientLimiter.cpp:126-129`) returns `mLookaheadSamples`, which is computed in `setLookahead()` as:

```cpp
mLookaheadSamples = static_cast<int>(clamped * 0.001f * static_cast<float>(mSampleRate));
```

`mSampleRate` is whatever rate was passed to `prepare()`. In production use via `LimiterEngine::prepare()`, TransientLimiter is prepared at the **upsampled** rate:

```cpp
mTransientLimiter.prepare(usSampleRate, usBlockSize, numChannels);  // e.g. 176400 Hz for 4x OS
```

At 1 ms lookahead with 4× oversampling at 44100 Hz:
- `usSampleRate` = 176400 Hz
- `getLatencyInSamples()` returns **176** (upsampled-domain samples)
- Correct original-rate latency = **44** samples

Any code that calls `transientLimiter.getLatencyInSamples()` expecting original-rate samples will overestimate latency by a factor of the oversampling ratio. `LimiterEngine::getLatencySamples()` currently avoids this by recomputing from `mLookaheadMs` and `mSampleRate` (original rate), bypassing `getLatencyInSamples()` entirely. However, this creates a latent bug risk: unit tests and future callers prepare TransientLimiter at the original rate, so tests pass, but the API semantics differ from production behaviour.

**Fix**: Change `TransientLimiter` to always store latency in original-rate samples, by storing `mLookaheadMs` and `mOriginalSampleRate` separately from `mSampleRate`. The internal `mLookaheadSamples` used for the delay buffer stays at the operating rate, but `getLatencyInSamples()` returns the original-rate value:

```cpp
// In setLookahead():
mLookaheadSamples = static_cast<int>(clamped * 0.001f * static_cast<float>(mSampleRate));  // operating rate

// New method (or renamed):
int getLatencyInOriginalRateSamples() const {
    return static_cast<int>(mLookaheadMs * 0.001f * static_cast<float>(mOriginalSampleRate));
}
```

Alternatively, update the existing `getLatencyInSamples()` to use original-rate calculation, since that is what all callers expect. Either rename `mSampleRate` to `mOperatingSampleRate` and add `mOriginalSampleRate`, or have `LimiterEngine` pass the original rate separately when calling `setLookahead()`.

Since `LimiterEngine` already correctly computes original-rate latency independently, the simplest safe fix is to add a `/** Returns latency in operating-rate samples. Divide by oversamplingFactor for original-rate latency. */` doc comment, but the correct fix is to make the API unambiguous.

## Produces
None

## Consumes
None

## Relevant Files
Modify: `M-LIM/src/dsp/TransientLimiter.h` — add `mOriginalSampleRate` member, update `getLatencyInSamples()` declaration
Modify: `M-LIM/src/dsp/TransientLimiter.cpp` — update `prepare()` to store original rate, update `getLatencyInSamples()` to return original-rate value
Read: `M-LIM/src/dsp/LimiterEngine.cpp` — verify `getLatencySamples()` logic (lines 597-603) still matches the corrected API

## Acceptance Criteria
- [ ] Run: `cd /workspace/M-LIM/build && ctest --output-on-failure` → Expected: all tests pass, exit 0
- [ ] After fix: `TransientLimiter` prepared at 176400 Hz (4× OS) with 1 ms lookahead reports `getLatencyInSamples() == 44` (original-rate samples at 44100 Hz), not 176.

## Tests
- Unit: `tests/dsp/test_transient_limiter.cpp::test_latency_in_original_rate_samples_when_prepared_at_upsampled_rate` — prepare TransientLimiter at 4× upsampled rate (176400 Hz) with 1 ms lookahead; call `getLatencyInSamples()` and verify it returns ~44 (original-rate equivalent).

## Technical Details
- `mOriginalSampleRate` must be passed to `prepare()` alongside the operating sample rate, or derived from the oversampling factor. The cleanest API change: `prepare(double operatingSampleRate, int maxBlockSize, int numChannels, double originalSampleRate = 0.0)` — if `originalSampleRate == 0`, treat as 1× (no oversampling).
- Unit tests that prepare TransientLimiter directly at 44100 Hz (no oversampling) are unaffected; they will continue to work correctly since `originalSampleRate == operatingSampleRate`.

## Dependencies
None
