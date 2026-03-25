# Task 195: Fix LoudnessMeter LRA Gate Threshold 1 LU Below EBU R128 Standard

## Description

In `LoudnessMeter::computeLRA()`, the absolute gate threshold used to filter 3-second windows into the LRA histogram is `-71 LUFS` — 1 LU below the EBU R128 / ITU-R BS.1770-4 specified gate of `-70 LUFS`.

**File:** `src/dsp/LoudnessMeter.cpp`, line 286:
```cpp
// CURRENT (wrong):
if (l > static_cast<float>(kAbsGateLUFS) - 1.0f)
```
`kAbsGateLUFS = -70.0`, so this evaluates to `l > -71.0`, which is 1 LU looser than the standard.

**Fix:** Remove the `- 1.0f` offset so the gate is exactly at `-70 LUFS`:
```cpp
// CORRECT (matches EBU R128 §4.6):
if (l > static_cast<float>(kAbsGateLUFS))
```

The 1 LU "numerical safety margin" comment has no basis in the standard. EBU R128 Tech 3342 §2.1 states the LRA gate is at `-70 LUFS` (no margin). Including near-silent passages between `-71` and `-70 LUFS` can inflate LRA measurements by pulling the lower percentile toward silence.

Note: `computeIntegratedLUFS()` correctly uses `kAbsGateLUFS` (−70 LUFS) without any offset — this inconsistency should be resolved by fixing LRA to match.

## Produces
None

## Consumes
None

## Relevant Files
Modify: `src/dsp/LoudnessMeter.cpp` — remove `- 1.0f` on line 286
Read: `src/dsp/LoudnessMeter.h` — context for constants and method declarations
Read: `tests/dsp/test_loudness_meter_accuracy.cpp` — extend LRA tests to verify gate threshold

## Acceptance Criteria
- [ ] Run: `grep "kAbsGateLUFS" src/dsp/LoudnessMeter.cpp` → Expected: gate comparison uses `kAbsGateLUFS` without subtraction in `computeLRA()`
- [ ] Run: `cd build && ctest -R test_loudness_meter --output-on-failure` → Expected: all loudness meter tests pass
- [ ] Run: `cd build && ctest --output-on-failure` → Expected: full test suite passes

## Tests
- Unit: `tests/dsp/test_loudness_meter_accuracy.cpp` — add a test case feeding a signal whose 3s-window LUFS is between -71 and -70 LUFS and verifying it is excluded from LRA (i.e., LRA stays 0 or -inf for a single-level signal near -70 LUFS gate).

## Technical Details
The EBU R128 Technical Specification (EBU Tech 3342, §2.1, LRA definition) specifies:
> "The absolute gate is applied at -70 LUFS."

The `computeIntegratedLUFS()` function already uses the correct gate:
```cpp
const double absGateLinear = lufsToLinear(static_cast<float>(kAbsGateLUFS));  // exact -70 LUFS
```

The `computeLRA()` function must use the same value. The `kAbsGateLUFS` constant in `LoudnessMeter.cpp` is already `-70.0` — simply use it directly.

## Dependencies
None
