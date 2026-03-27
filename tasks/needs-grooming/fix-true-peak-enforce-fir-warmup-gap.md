# Task: Fix True Peak Enforcement FIR Warm-Up Gap at Block Boundaries

## Description
In `LimiterEngine::stepEnforceTruePeak()` (line 358-361), when a true peak overshoot is detected and corrective gain is applied, the enforcement detectors are fully reset via `mTruePeakEnforceL.reset()` / `mTruePeakEnforceR.reset()`. This clears the FIR filter history (12 samples of state), meaning the first ~12 samples of the next audio block will have degraded inter-sample peak detection accuracy due to the cold FIR startup.

**The issue**: During this warm-up period, inter-sample peaks can slip through undetected, potentially exceeding the output ceiling. For a professional mastering limiter claiming ITU-R BS.1770-4 compliance, this is a measurable accuracy gap.

**Fix approach**: Instead of `reset()`, use `resetPeak()` (which already exists and preserves FIR state), then apply the corrective gain and reprocess the block through the detector to update the FIR state with the corrected signal. Alternatively, scale the FIR history state by the same gain factor applied to the audio buffer, which maintains time-continuity of the filter state.

**Current code** (`LimiterEngine.cpp:356-361`):
```cpp
mTruePeakEnforceL.reset();   // clears FIR state unnecessarily
if (numChannels > 1)
    mTruePeakEnforceR.reset();
```

**Better approach**: After applying corrective gain, reprocess the corrected buffer through the enforcement detectors so the FIR state matches the actual output:
```cpp
mTruePeakEnforceL.resetPeak();
mTruePeakEnforceL.processBlock(buffer.getReadPointer(0), numSamples);
if (numChannels > 1) {
    mTruePeakEnforceR.resetPeak();
    mTruePeakEnforceR.processBlock(buffer.getReadPointer(1), numSamples);
}
```

## Produces
None

## Consumes
None

## Relevant Files
Modify: `M-LIM/src/dsp/LimiterEngine.cpp` — stepEnforceTruePeak() around line 356-361
Read: `M-LIM/src/dsp/TruePeakDetector.h` — resetPeak() vs reset() semantics

## Acceptance Criteria
- [ ] Run: `cd build && ctest --output-on-failure` → Expected: all tests pass, exit 0
- [ ] Run: verification that true peak enforcement detector FIR state is continuous across blocks when enforcement triggers

## Tests
- Unit: `tests/dsp/test_limiter_engine.cpp::test_true_peak_enforce_fir_continuity` — generate a signal that triggers true peak enforcement at a block boundary, verify next block's true peak detection is accurate from sample 0

## Technical Details
- The reprocessing adds ~12 FIR multiplies per enforcement event (once per block, only when overshoot detected) — negligible CPU cost
- This ensures the FIR ring buffer contains the corrected (post-gain) samples, not stale pre-correction data

## Dependencies
None
