# Task: NaN/Inf Input Robustness Tests for LevelingLimiter and TransientLimiter

## Description
Neither `LevelingLimiter` nor `TransientLimiter` has any tests verifying behavior when
NaN or Inf samples arrive in the input buffer. In a real DAW chain a broken upstream
plugin can produce NaN/Inf audio; the limiter must not crash or invoke UB (e.g. from
storing NaN into its IIR state and propagating it indefinitely).

Current state:
- `test_leveling_limiter.cpp` has **zero** uses of `std::isnan`/`std::isinf`/`std::isfinite`.
- `test_transient_limiter.cpp` checks `std::isfinite` only for normal outputs, not for
  responses to intentionally-injected NaN/Inf inputs.
- `test_dc_filter.cpp` similarly has no NaN/Inf injection tests.

Add tests for:
1. **LevelingLimiter**: process a block containing `std::numeric_limits<float>::quiet_NaN()`
   in all samples. Verify: no crash, no assertion failure, output is either all NaN/Inf
   (propagation) or clamped/zero (sanitised) — whichever the implementation guarantees —
   but the *other* channel must not be corrupted if the component is genuinely per-channel.
2. **LevelingLimiter**: same with `std::numeric_limits<float>::infinity()` and `-infinity`.
3. **TransientLimiter**: inject NaN block into one channel while the other channel carries a
   valid 0 dBFS sine. Verify: the valid channel's gain reduction is finite (the NaN
   channel's contamination does not propagate through the linked-channel gain logic to
   produce NaN GR on the clean channel, or at minimum no crash).
4. **DCFilter**: inject NaN/Inf → verify no crash; filter state after may be NaN (expected),
   but a subsequent `reset()` followed by valid audio must restore finite output within
   a reasonable settling period (i.e. reset actually clears the contaminated state).

## Produces
None

## Consumes
None

## Relevant Files
Read: `M-LIM/tests/dsp/test_leveling_limiter.cpp` — add new TEST_CASE blocks here
Read: `M-LIM/tests/dsp/test_transient_limiter.cpp` — add new TEST_CASE blocks here
Read: `M-LIM/tests/dsp/test_dc_filter.cpp` — add new TEST_CASE blocks here
Read: `M-LIM/src/dsp/LevelingLimiter.h` — understand process() signature
Read: `M-LIM/src/dsp/TransientLimiter.h` — understand process() signature
Read: `M-LIM/src/dsp/DCFilter.h` — understand process() and reset() signatures

## Acceptance Criteria
- [ ] Run: `cd build && ctest -R "LevelingLimiter|TransientLimiter|DCFilter" --output-on-failure` → Expected: all tests pass, exit 0
- [ ] Run: `grep -c "nan\|NaN\|infinity\|numeric_limits" M-LIM/tests/dsp/test_leveling_limiter.cpp` → Expected: output >= 5 (at least 5 new references)
- [ ] Run: `grep -c "nan\|NaN\|infinity\|numeric_limits" M-LIM/tests/dsp/test_transient_limiter.cpp` → Expected: output >= 3

## Tests
- Unit: `tests/dsp/test_leveling_limiter.cpp::test_nan_input_no_crash` — NaN block in, no crash
- Unit: `tests/dsp/test_leveling_limiter.cpp::test_inf_input_no_crash` — Inf block in, no crash
- Unit: `tests/dsp/test_leveling_limiter.cpp::test_nan_channel_isolated_from_clean_channel` — NaN L does not corrupt R
- Unit: `tests/dsp/test_transient_limiter.cpp::test_nan_block_no_crash` — NaN block in, no crash/UB
- Unit: `tests/dsp/test_transient_limiter.cpp::test_nan_channel_does_not_corrupt_other_channel` — one NaN channel, verify linked GR finite
- Unit: `tests/dsp/test_dc_filter.cpp::test_nan_input_no_crash` — NaN in, no crash
- Unit: `tests/dsp/test_dc_filter.cpp::test_reset_after_nan_restores_finite_output` — reset clears NaN state

## Technical Details
- Use `std::numeric_limits<float>::quiet_NaN()` and `std::numeric_limits<float>::infinity()`.
- Prepare the components at 44100 Hz, 512 samples before injecting NaN/Inf.
- The tests verify no crash/UB, not necessarily that output is finite (propagation is acceptable as long as it's deterministic). Document which behavior the implementation actually exhibits.
- For the channel isolation test: set both channels to the same input but fill only ch0 with NaN; check that `getGainReduction()` is finite after processing (or add a comment if GR is also contaminated, since that would be a separate bug to fix).

## Dependencies
None
