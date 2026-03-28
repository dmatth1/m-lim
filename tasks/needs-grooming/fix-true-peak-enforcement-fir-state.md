# Task: Fix true peak enforcement stale FIR state at block boundaries

## Description
In `LimiterEngine::stepEnforceTruePeak()`, when the worst true peak exceeds the ceiling, the code applies a uniform gain correction and then rescans the corrected buffer using `resetPeak()` + `processBlock()`. However, `resetPeak()` preserves the FIR filter state from the *first* scan (which processed the un-corrected signal). This means the first ~12 samples of the second scan produce FIR outputs that mix stale history (from the louder pre-correction signal) with current samples (from the quieter post-correction signal). This can cause the second-pass true peak measurement to be slightly inaccurate at the block boundary, potentially allowing sub-dB true peak overshoot or unnecessary over-correction.

## Produces
None

## Consumes
None

## Relevant Files
Modify: `M-LIM/src/dsp/LimiterEngine.cpp` — `stepEnforceTruePeak()` method (lines 356–384)
Read: `M-LIM/src/dsp/TruePeakDetector.h` — FIR state management, `resetPeak()` vs `reset()`
Read: `M-LIM/src/dsp/TruePeakDetector.cpp` — `processSample()` linear buffer mechanics

## Acceptance Criteria
- [ ] Run: `cd build && ctest --output-on-failure -R true_peak` → Expected: all tests pass, exit 0
- [ ] Run: `cd build && ctest --output-on-failure` → Expected: all tests pass, exit 0

## Tests
- Unit: `tests/dsp/test_true_peak.cpp::test_enforcement_block_boundary_accuracy` — feed a signal with a peak exactly at a block boundary, verify the enforced output true peak is within 0.1 dB of the ceiling

## Technical Details
**Recommended fix approach**: Use a separate pair of `TruePeakDetector` instances for the enforcement re-scan (already exists as `mTruePeakEnforcers`). The issue is that the same enforcer instances are used for both the detection scan and the verification re-scan. Options:
1. **Preferred**: After applying gain correction, call `reset()` (not `resetPeak()`) on the enforcers before the re-scan. This clears FIR state entirely, accepting a ~12-sample warm-up gap in exchange for accurate peak measurement. The warm-up gap is acceptable since we only care about the *peak* of the re-scanned block, and the warm-up samples will be attenuated (reading zero history = lower output).
2. **Alternative**: Maintain a third set of `TruePeakDetector` instances dedicated to the verification re-scan, so the main enforcers' FIR state is never polluted.

The magnitude of the error is small (proportional to the gain correction factor, typically a few tenths of a dB), so this is low priority.

## Dependencies
None
