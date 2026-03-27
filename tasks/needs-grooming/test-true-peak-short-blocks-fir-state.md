# Task: TruePeakDetector — Very Short Block FIR State and Ring Buffer Edge Cases

## Description
TruePeakDetector uses a polyphase FIR filter to detect inter-sample peaks. The following edge cases are untested:

1. **1-sample blocks**: Processing a single sample per call exercises the ring buffer's minimum case. If the ring buffer read-ahead indexing has an off-by-one, 1-sample blocks will expose it while larger blocks mask it.

2. **Ring buffer wrap-around glitch**: Feed a sequence of sine waves across many small blocks totaling more than the FIR delay line length. At the wrap-around point, the output should be continuous — no sample discontinuity.

3. **reset() vs resetPeak() state independence**: `resetPeak()` clears the tracked peak but preserves FIR state. `reset()` clears both. Tests exist but do not verify that after `resetPeak()`, the FIR continues to produce correct peak detection on the next block (i.e., FIR state is truly preserved and not partially cleared).

4. **Negative input handled correctly**: True peak should track the maximum absolute value. A block of all negative samples (-0.9) should produce a peak of 0.9, not -0.9. No test verifies this.

## Produces
None

## Consumes
None

## Relevant Files
Read: `M-LIM/src/dsp/TruePeakDetector.h` — `processBlock()`, `reset()`, `resetPeak()`, ring buffer
Read: `M-LIM/src/dsp/TruePeakDetector.cpp` — FIR implementation
Read: `M-LIM/tests/dsp/test_true_peak.cpp` — existing tests
Modify: `M-LIM/tests/dsp/test_true_peak.cpp` — add new tests

## Acceptance Criteria
- [ ] Run: `cd build && ctest -R "TruePeak" --output-on-failure` → Expected: all tests pass, exit 0

## Tests
- Unit: `tests/dsp/test_true_peak.cpp::test_single_sample_blocks_over_1000_calls` — Process 1000 single-sample blocks of a 1 kHz sine (samples computed at their correct phase positions). After all blocks, `getPeak()` should be >= the sample-domain peak (true peak >= sample peak for sine waves, by definition).
- Unit: `tests/dsp/test_true_peak.cpp::test_ring_buffer_wrap_continuity` — Feed a 1 kHz sine across many 7-sample blocks for 4096 total samples. Verify there is no amplitude discontinuity at any block boundary (consecutive output samples differ by no more than the expected sine delta + small tolerance).
- Unit: `tests/dsp/test_true_peak.cpp::test_reset_peak_preserves_fir_state` — Process 100 samples of a sine to settle FIR. Call `resetPeak()`. Process 100 more samples. Verify peak is >= 0 (FIR didn't go NaN or corrupt) and equals the expected true peak of the second block of sine.
- Unit: `tests/dsp/test_true_peak.cpp::test_negative_input_tracked_as_absolute` — Feed a block of all-negative samples at amplitude -0.8. `getPeak()` should return a value >= 0.8 (absolute value tracked), not a negative peak.

## Technical Details
- For the ring buffer continuity test, the block size of 7 is chosen because it's coprime with typical FIR lengths (2x/4x oversampled detectors use 4-tap or 8-tap FIR), maximizing coverage of partial-block edge cases.
- The 1-sample test is a stress test — 1000 iterations ensure ring buffer indices cycle through all positions.

## Dependencies
None
