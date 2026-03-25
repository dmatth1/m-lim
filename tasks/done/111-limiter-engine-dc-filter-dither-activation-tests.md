# Task 111: LimiterEngine DC Filter and Dither Activation Integration Tests

## Description
`DCFilter` and `Dither` are tested in isolation but there are **no tests** that verify they
are correctly wired into `LimiterEngine` and activated by `setDCFilterEnabled()` /
`setDitherEnabled()`. If the engine's `process()` method contained a bug (e.g., checking the
wrong atomic flag or applying filters to the wrong channel), the isolation tests would still
pass while the engine would produce incorrect output.

Add integration tests to `tests/dsp/test_limiter_engine.cpp` (or a new file
`tests/dsp/test_limiter_engine_dc_dither.cpp`) that verify:

1. **DC filter activation removes DC offset from engine output**: Feed a signal with a
   significant DC bias through the engine. With `setDCFilterEnabled(false)`, the output
   should retain a measurable DC component. With `setDCFilterEnabled(true)`, the output
   should converge toward DC-free after a few blocks.

2. **DC filter is applied to both channels**: Verify DC removal occurs on both L and R
   channels (not just channel 0).

3. **Dither activation adds noise floor**: Feed silence (0.0f) through the engine. With
   `setDitherEnabled(false)`, the output should be exactly zero. With `setDitherEnabled(true)`
   and `setDitherBitDepth(16)`, the output should contain a small but non-zero noise floor
   (at least a few samples are non-zero across 4096-sample run).

4. **Dither does not clip the ceiling**: With dither enabled, output peaks must still remain
   at or below the configured ceiling, since dither noise is below the quantisation step.

## Produces
None

## Consumes
None

## Relevant Files
Read: `src/dsp/LimiterEngine.h` — setDCFilterEnabled, setDitherEnabled, setDitherBitDepth
Read: `src/dsp/LimiterEngine.cpp` — steps 8 and 9 of process()
Read: `src/dsp/DCFilter.h` — used for understanding expected behavior
Read: `src/dsp/Dither.h` — used for understanding expected behavior
Modify: `tests/dsp/test_limiter_engine.cpp` — add 4 tests

## Acceptance Criteria
- [ ] Run: `cd /workspace/M-LIM/build && ctest -R "LimiterEngine" --output-on-failure` → Expected: all tests pass
- [ ] Run: `cd /workspace/M-LIM/build && ctest --output-on-failure 2>&1 | grep -E "passed|failed"` → Expected: no regressions

## Tests
- Unit: `tests/dsp/test_limiter_engine.cpp::test_dc_filter_removes_dc_when_enabled` — measure DC in output with enabled vs. disabled; enabled must produce near-zero DC
- Unit: `tests/dsp/test_limiter_engine.cpp::test_dc_filter_both_channels` — both L and R channels have DC removed
- Unit: `tests/dsp/test_limiter_engine.cpp::test_dither_adds_noise_when_enabled` — silence input produces zero output with dither off, non-zero with dither on
- Unit: `tests/dsp/test_limiter_engine.cpp::test_dither_stays_below_ceiling` — dithered output peak ≤ ceiling + epsilon

## Technical Details
DC test: Input signal = 0.5f (DC only, no AC). After prepare(), run 200 blocks of 512 samples.
Measure mean of last block — should be near 0.0 with DC filter enabled.

Dither test: Input = 0.0f for 4096 samples, dither enabled at 16-bit. Count non-zero output
samples — must be > 0 (conservative: at least 100 out of 4096).

Note: To avoid DC filter settling time, warm up with 200 blocks before measuring (DC filter
is first-order high-pass at ~10 Hz, settling time ~100ms).

## Dependencies
None
