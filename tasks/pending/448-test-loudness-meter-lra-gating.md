# Task 448: LoudnessMeter — LRA Computation and Absolute/Relative Gating Tests

## Description
LoudnessMeter's `getLoudnessRange()` is completely untested. Missing tests:
1. LRA of constant signal ≈ 0
2. LRA of alternating loud/quiet ≈ 10 LU
3. Absolute gate (-70 LUFS) excludes quiet blocks from integrated
4. resetIntegrated() discards accumulated history
5. Relative gating: loud burst dominates over long silence

## Produces
None

## Consumes
None

## Relevant Files
Read: `src/dsp/LoudnessMeter.h` — getLoudnessRange(), resetIntegrated(), gating constants
Read: `src/dsp/LoudnessMeter.cpp` — gating implementation
Read: `tests/dsp/test_loudness_meter_accuracy.cpp` — existing coverage
Modify: `tests/dsp/test_loudness_meter_accuracy.cpp` — add new tests

## Acceptance Criteria
- [ ] Run: `cd build && ctest -R "LoudnessMeter" --output-on-failure` → Expected: all tests pass, exit 0

## Tests
- Unit: `tests/dsp/test_loudness_meter_accuracy.cpp::test_lra_constant_signal_zero` — 10 s at -23 LUFS; getLoudnessRange() < 0.5 LU
- Unit: `tests/dsp/test_loudness_meter_accuracy.cpp::test_lra_alternating_loud_quiet` — alternating -23/-33 LUFS (30 s total); LRA in [8.0, 12.0] LU
- Unit: `tests/dsp/test_loudness_meter_accuracy.cpp::test_absolute_gate_excludes_quiet_blocks` — 5 s at -80 LUFS; getIntegratedLUFS() returns -inf sentinel
- Unit: `tests/dsp/test_loudness_meter_accuracy.cpp::test_reset_integrated_discards_history` — 5 s loud, resetIntegrated(), 0.5 s fresh; integrated LUFS reflects only post-reset signal
- Unit: `tests/dsp/test_loudness_meter_accuracy.cpp::test_relative_gating_loud_burst_dominates` — 20 s silence + 5 s -23 LUFS burst; integrated LUFS close to -23 LUFS

## Technical Details
Verify exact method name from LoudnessMeter.h. Tolerance: ±1.5 LU. Fix task 432
(LRA off-by-one) before or alongside this task for correct results.

## Dependencies
None
