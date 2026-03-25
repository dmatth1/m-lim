# Task 142: LoudnessMeter — Missing Tests for reset() and Silence-Only Behaviour

## Description
`LoudnessMeter` accumulates integrated LUFS over time. Two important behaviours have no
test coverage:

**1. reset() clears integrated LUFS:**
`LoudnessMeter::reset()` (if it exists) or `prepare()` should reset the integrated LUFS
accumulator so a fresh measurement begins. No test verifies that after calling reset() or
re-preparing, the integrated LUFS no longer reflects earlier audio.

**2. Silence-only input does not return NaN:**
When the meter processes only silence for its full window duration (400 ms for momentary,
3 s for short-term), the window is below the gating threshold. The BS.1770 gating algorithm
should return a meaningful sentinel (typically -inf or the gating threshold equivalent), not
NaN or +inf. No test covers the all-silence case.

Existing tests only feed audio signals (e.g., test_loudness_metering_active feeds a -6 dBFS
sine) and check for finite non-silence values. The edge case of pure silence is untested.

## Produces
None

## Consumes
None

## Relevant Files
Read: `M-LIM/src/dsp/LoudnessMeter.h` — public API: reset(), processBlock(), getMomentaryLUFS(), getShortTermLUFS(), getIntegratedLUFS(), getLoudnessRange()
Read: `M-LIM/src/dsp/LoudnessMeter.cpp` — gating logic, accumulator reset
Modify: `M-LIM/tests/dsp/test_loudness_meter.cpp` — add new test cases

## Acceptance Criteria
- [ ] Run: `cd /workspace/M-LIM/build && ctest -R LoudnessMeter --output-on-failure` → Expected: all tests pass, exit 0

## Tests
- Unit: `tests/dsp/test_loudness_meter.cpp::test_silence_not_nan` — prepare at 48000 Hz,
  process 5 seconds of all-zero stereo audio in 512-sample blocks; assert that
  `getMomentaryLUFS()`, `getShortTermLUFS()`, and `getIntegratedLUFS()` are all either
  `std::isfinite()` or exactly `-std::numeric_limits<float>::infinity()` — never NaN.
- Unit: `tests/dsp/test_loudness_meter.cpp::test_reset_clears_integrated` — feed 3 seconds
  of a -18 LUFS sine so integrated LUFS is a real value; call `reset()` (or `prepare()`);
  feed 0.5 seconds of a much louder signal; verify that integrated LUFS reflects only the
  new signal, not a mixture with the pre-reset audio.
- Unit: `tests/dsp/test_loudness_meter.cpp::test_reprepare_clears_state` — call
  `prepare(48000, 2)`, feed 2 seconds of a loud tone, then call `prepare(48000, 2)` again
  (simulating a DAW restart), feed silence; verify integrated LUFS is not inherited from
  before the second prepare.

## Technical Details
- BS.1770-4 gating: the integrated LUFS measurement requires at least one 400 ms gate
  block above -70 LUFS relative threshold. With only silence, no gate blocks qualify and
  the return value is implementation-defined. The test only requires it not be NaN.
- If `LoudnessMeter` lacks a standalone `reset()` method and relies on `prepare()` for
  resetting state, use `prepare()` in the test and document this in the test comment.
- Use `feedSilence()` helper pattern from `test_loudness_meter_accuracy.cpp` for
  consistency with existing test infrastructure.

## Dependencies
None
