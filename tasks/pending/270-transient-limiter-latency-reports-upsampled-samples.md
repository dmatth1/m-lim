# Task 270: Fix TransientLimiter::getLatencyInSamples() returning wrong domain samples

## Description

`TransientLimiter::getLatencyInSamples()` returns `mLookaheadSamples`, which is computed using the **upsampled** sample rate (the limiter is always `prepare()`d with `usSampleRate = sampleRate * oversamplingFactor` from `LimiterEngine`).

The docstring on `TransientLimiter.h:52–55` explicitly claims:
> "This equals the plugin latency that must be reported to the host via `AudioProcessor::setLatencySamples()`."

This claim is **wrong**. When oversampling is active (e.g., 4× OS at 44.1 kHz), `mLookaheadSamples` = `lookaheadMs × 0.001 × 176400`, which is 4× the correct host-facing latency in original-domain samples.

`LimiterEngine::getLatencySamples()` (the actual host-facing path) correctly recomputes from `lookaheadMs × 0.001 × mSampleRate` (original rate), so there is no audio-thread bug today. The danger is:
1. The misleading docstring could cause future developers to misuse the method.
2. Test code in `tests/dsp/test_transient_limiter.cpp` validates the return value only against a non-oversampled `prepare()`, masking the domain mismatch.

## Produces
None

## Consumes
None

## Relevant Files
Read: `M-LIM/src/dsp/TransientLimiter.h` — header with misleading docstring on `getLatencyInSamples()`
Modify: `M-LIM/src/dsp/TransientLimiter.h` — fix the docstring
Read: `M-LIM/src/dsp/TransientLimiter.cpp` — verify `mLookaheadSamples` is upsampled-rate
Read: `M-LIM/src/dsp/LimiterEngine.cpp` — see correct latency computation at `getLatencySamples()`
Read: `M-LIM/tests/dsp/test_transient_limiter.cpp` — tests at line 520–572 test with original rate only

## Acceptance Criteria
- [ ] Run: `grep "getLatencyInSamples" M-LIM/src/dsp/TransientLimiter.h` → Expected: docstring no longer claims the return value equals plugin latency for host reporting; it states the return is in upsampled-rate samples
- [ ] Run: `cd M-LIM/build && ctest -R test_lookahead_latency --output-on-failure` → Expected: all latency tests pass

## Tests
None (documentation fix only; existing tests continue to pass)

## Technical Details

Fix: Update the docstring for `getLatencyInSamples()` in `TransientLimiter.h` to read:

```cpp
/** Returns the lookahead delay in samples **at the rate this instance was prepared with**.
 *  When the limiter is prepared at an oversampled rate (as done by LimiterEngine),
 *  this value is in upsampled-domain samples, NOT original-rate samples.
 *  Do NOT use this value directly for host latency reporting.
 *  Use LimiterEngine::getLatencySamples() for the host-facing latency. */
int getLatencyInSamples() const;
```

No code logic changes are required — the docstring fix alone closes the risk.

## Dependencies
None
