# Task: LoudnessMeter — LRA Computation and Absolute Gating Tests

## Description
The existing loudness tests verify momentary/short-term/integrated LUFS values but do not test:
1. **Loudness Range (LRA)** — the `getLoudnessRange()` method is completely untested. EBU R128 defines LRA as the difference between the 10th and 95th percentile of gated short-term blocks. A silent signal should give LRA = 0; a signal alternating between -23 LUFS and -33 LUFS should give LRA ≈ 10 LU.
2. **Absolute gating** — integrated LUFS must ignore blocks below -70 LUFS (absolute gate). No test verifies a very quiet signal (< -70 LUFS) does not contribute to integrated LUFS.
3. **Relative gating** — integrated LUFS ignores blocks more than 10 LU below the preliminary mean. No test verifies this (a loud burst followed by long silence should have integrated LUFS driven by the burst, not pulled down by silence).
4. **resetIntegrated()** — verified to exist but not that subsequent measurement after reset starts fresh (accumulated history discarded, not just counter reset).

## Produces
None

## Consumes
None

## Relevant Files
Read: `M-LIM/src/dsp/LoudnessMeter.h` — `getLoudnessRange()`, `resetIntegrated()`, gating constants
Read: `M-LIM/src/dsp/LoudnessMeter.cpp` — gating implementation details
Modify: `M-LIM/tests/dsp/test_loudness_meter_accuracy.cpp` — add new test cases

## Acceptance Criteria
- [ ] Run: `cd build && ctest -R "LoudnessMeter" --output-on-failure` → Expected: all tests pass, exit 0

## Tests
- Unit: `tests/dsp/test_loudness_meter_accuracy.cpp::test_lra_constant_signal_zero` — Feed 10 s of constant -23 LUFS sine; `getLoudnessRange()` should be < 0.5 LU (all blocks identical).
- Unit: `tests/dsp/test_loudness_meter_accuracy.cpp::test_lra_alternating_loud_quiet` — Alternate 5 s at -23 LUFS and 5 s at -33 LUFS (total 30 s); `getLoudnessRange()` should be within [8.0, 12.0] LU.
- Unit: `tests/dsp/test_loudness_meter_accuracy.cpp::test_absolute_gate_excludes_quiet_blocks` — Feed 5 s at -80 LUFS (below absolute gate of -70 LUFS); `getIntegratedLUFS()` should return `-inf` (or the sentinel value for "not enough data").
- Unit: `tests/dsp/test_loudness_meter_accuracy.cpp::test_reset_integrated_discards_history` — Feed 5 s loud signal. Call `resetIntegrated()`. Feed 0.5 s of fresh signal. Verify `getIntegratedLUFS()` reflects only the post-reset measurement (not pulled toward old history).
- Unit: `tests/dsp/test_loudness_meter_accuracy.cpp::test_relative_gating_loud_burst_dominates` — Feed 20 s silence then 5 s -23 LUFS burst. Integrated LUFS should be close to -23 LUFS (silence gated out), not pulled down by 20 s of silence.

## Technical Details
- LRA getter: check `M-LIM/src/dsp/LoudnessMeter.h` for exact method name (`getLoudnessRange()` per the comment in the class declaration; confirm before writing tests).
- Tolerance for LRA tests: ±1.5 LU is acceptable given 100 ms block resolution.
- ITU absolute gate = -70 LUFS; relative gate = 10 LU below ungated mean.

## Dependencies
None
