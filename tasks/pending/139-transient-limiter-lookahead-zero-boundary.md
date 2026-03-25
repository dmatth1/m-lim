# Task 139: TransientLimiter — Missing Tests for Lookahead=0 Boundary Case

## Description
`TransientLimiter::setLookahead(0.0f)` should result in `getLatencyInSamples()` returning 0 and
the limiter still correctly clamping peaks without introducing any delay compensation.
No test currently covers this boundary:

- `test_peak_limiting` uses `setLookahead(1.0f)` (1 ms).
- `test_lookahead_anticipation` requires a non-zero lookahead.
- There are zero tests that call `setLookahead(0.0f)` and then verify limiting still works.

Bugs that could hide here:
1. The delay buffer might not collapse to zero length, introducing one extra sample of silence
   at startup or a silent first sample in output.
2. `getLatencyInSamples()` might return a stale previous value rather than 0.
3. The deque-based peak scanner might behave incorrectly with a 0-length window,
   causing infinite GR or zero GR.

## Produces
None

## Consumes
None

## Relevant Files
Read: `M-LIM/src/dsp/TransientLimiter.h` — public API; setLookahead, getLatencyInSamples
Read: `M-LIM/src/dsp/TransientLimiter.cpp` — implementation details of the 0-lookahead code path
Modify: `M-LIM/tests/dsp/test_transient_limiter.cpp` — add the new test cases

## Acceptance Criteria
- [ ] Run: `cd /workspace/M-LIM/build && ctest -R TransientLimiter --output-on-failure` → Expected: all tests pass, exit 0

## Tests
- Unit: `tests/dsp/test_transient_limiter.cpp::test_lookahead_zero_no_latency` — call
  `setLookahead(0.0f)`, verify `getLatencyInSamples() == 0`.
- Unit: `tests/dsp/test_transient_limiter.cpp::test_lookahead_zero_still_limits` — with
  `setLookahead(0.0f)`, feed a block of +6 dBFS constant signal, verify output peak ≤ 1.0 + margin
  after a few warm-up blocks.
- Unit: `tests/dsp/test_transient_limiter.cpp::test_lookahead_zero_to_nonzero_transition` —
  start with `setLookahead(0.0f)`, process 5 blocks, then call `setLookahead(1.0f)` and verify
  `getLatencyInSamples()` updates to a non-zero value and limiting still works.

## Technical Details
- The 0-lookahead path bypasses the circular delay buffer: the output should be the
  current input sample multiplied by the gain computed from the current sample itself.
- With no lookahead, GR is applied instantaneously — no anticipation. A single constant
  block of 2.0f should be reduced to ≤ 1.0 on every sample (no warm-up delay).
- After calling `setLookahead(0.0f)` followed by `setLookahead(1.0f)`, the delay buffer
  should re-initialise to the new lookahead size without memory corruption.

## Dependencies
None
