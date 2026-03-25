# Task 148: TruePeakDetector — No Consistency Test Between processBlock and processSample

## Description
`TruePeakDetector` exposes two processing APIs:
- `processBlock(const float* data, int numSamples)` — batch processing
- `processSample(float x)` — single-sample processing with return value

No test verifies that these two methods produce consistent results when fed the same input
data. If the batch path has a different FIR windowing or index calculation from the
per-sample path, they could silently produce different peak values.

For example: `processBlock` might use SIMD or a different loop unrolling that causes
off-by-one access in the delay line, giving a different interpolated peak than `processSample`.

This inconsistency would affect:
1. `LimiterEngine` which calls `mTruePeakL.processSample()` on each channel sample during
   in-line processing, but `processBlock()` might be used elsewhere.
2. Test assertions in `test_true_peak.cpp` that call `processSample()` for setup and
   then call `getPeak()` — if `processBlock` and `processSample` diverge, the test results
   would not generalise to the real code paths.

## Produces
None

## Consumes
None

## Relevant Files
Read: `M-LIM/src/dsp/TruePeakDetector.h` — both processing APIs
Read: `M-LIM/src/dsp/TruePeakDetector.cpp` — implementation of processBlock vs processSample
Modify: `M-LIM/tests/dsp/test_true_peak.cpp` — add consistency test

## Acceptance Criteria
- [ ] Run: `cd /workspace/M-LIM/build && ctest -R TruePeakDetector --output-on-failure` → Expected: all tests pass, exit 0

## Tests
- Unit: `tests/dsp/test_true_peak.cpp::test_processblock_vs_processsample_same_peak` —
  Create two `TruePeakDetector` instances with identical `prepare()` state. Process the same
  256-sample Nyquist/4 sine wave through one via `processBlock()` and the other via a loop
  calling `processSample()`. After processing, call `getPeak()` on both and verify they agree
  within ±0.001 (allowing for minor floating-point ordering differences).
- Unit: `tests/dsp/test_true_peak.cpp::test_processblock_returns_correct_peak_object` —
  Verify that after `processBlock()` on a sine with known amplitude (0.9f), `getPeak()` is
  >= 0.9f (the inter-sample peak should be at least as large as the sample peak), confirming
  that `processBlock` actually updates the internal peak state.

## Technical Details
- Reset both detectors with `reset()` before the consistency test to ensure identical
  initial state (all FIR taps at zero).
- The Nyquist/4 sine (`freq = sampleRate / 4`) is the same stimulus already used in
  `test_detects_intersample_peak` — reuse it for consistency.
- A tolerance of ±0.001 in linear peak value accommodates legitimate floating-point
  ordering differences but catches structural discrepancies.

## Dependencies
None
