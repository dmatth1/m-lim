# Task: TransientLimiter — Lookahead Boundary and Multi-Prepare Edge Cases

## Description
Several untested edge cases in TransientLimiter could cause silent bugs in production:

1. **Lookahead buffer wrap-around at exact capacity**: The circular lookahead buffer has a maximum depth. A test should feed blocks whose total sample count equals exactly `maxLookahead` and then `maxLookahead + 1` to verify the wrap-around logic is correct (no sample skipped or duplicated).

2. **Multiple prepare() calls without destroy**: `prepare()` may be called more than once (e.g., host changes sample rate). No test verifies that the second `prepare()` correctly resets lookahead buffer indices, gain state, and does not leak the previous buffer. A fresh prepare followed by an impulse should produce a clean, single peak — not artifacts from the prior session.

3. **Zero-length block handling**: `process(buffer, 0)` should be a no-op — no state mutation, no crash. Subsequent blocks should process normally.

4. **Sidechain channel count mismatch**: If the sidechain buffer has fewer channels than the main buffer, the current code's behavior is untested and may index out-of-bounds.

## Produces
None

## Consumes
None

## Relevant Files
Read: `M-LIM/src/dsp/TransientLimiter.h` — lookahead buffer, `prepare()` signature
Read: `M-LIM/src/dsp/TransientLimiter.cpp` — ring buffer implementation
Read: `M-LIM/tests/dsp/test_transient_limiter.cpp` — existing tests to avoid duplication
Modify: `M-LIM/tests/dsp/test_transient_limiter.cpp` — add new tests at bottom

## Acceptance Criteria
- [ ] Run: `cd build && ctest -R "TransientLimiter" --output-on-failure` → Expected: all tests pass, exit 0

## Tests
- Unit: `tests/dsp/test_transient_limiter.cpp::test_lookahead_wrap_around_exact_capacity` — Set lookahead to its maximum. Feed `maxLookahead` samples of silence then 1 sample at amplitude 2.0, then silence. Assert the peak appears in the output at the correct delay position (not off by one from wrap-around).
- Unit: `tests/dsp/test_transient_limiter.cpp::test_reprepare_clears_state` — Call `prepare()` twice with the same parameters. Feed an impulse after the second prepare. Verify only one output peak exists (no artifact from residual prior state in the buffer).
- Unit: `tests/dsp/test_transient_limiter.cpp::test_zero_length_block_noop` — Call `process(buffer, 0)` immediately after prepare. Then feed a known signal; verify output matches what would happen if zero-length call never occurred.
- Unit: `tests/dsp/test_transient_limiter.cpp::test_sidechain_mono_main_stereo_no_crash` — Provide a mono sidechain buffer with a stereo main buffer. Must not crash; main audio should be processed (behavior for the mismatch is acceptable, crash is not).

## Technical Details
- Get max lookahead samples from the class (likely a compile-time constant or set at prepare() time — read the header).
- For wrap-around test: the exact capacity boundary is the failure mode; test `N-1`, `N`, and `N+1` samples of lookahead depth.

## Dependencies
None
