# Task 447: TruePeakDetector — Short Block FIR State and Ring Buffer Edge Cases

## Description
Missing TruePeakDetector tests:
1. 1-sample blocks over 1000 calls — ring buffer off-by-one detection
2. Ring buffer wrap-around continuity (7-sample blocks, 4096 total samples)
3. resetPeak() preserves FIR state — subsequent detection still correct
4. Negative input tracked as absolute value

## Produces
None

## Consumes
None

## Relevant Files
Read: `src/dsp/TruePeakDetector.h` — processBlock(), reset(), resetPeak(), ring buffer
Read: `src/dsp/TruePeakDetector.cpp` — FIR implementation
Read: `tests/dsp/test_true_peak.cpp` — existing tests
Modify: `tests/dsp/test_true_peak.cpp` — add new tests

## Acceptance Criteria
- [ ] Run: `cd build && ctest -R "TruePeak" --output-on-failure` → Expected: all tests pass, exit 0

## Tests
- Unit: `tests/dsp/test_true_peak.cpp::test_single_sample_blocks_over_1000_calls` — 1000 single-sample blocks of 1 kHz sine; getPeak() >= sample-domain peak
- Unit: `tests/dsp/test_true_peak.cpp::test_ring_buffer_wrap_continuity` — 7-sample blocks, 4096 total samples of 1 kHz sine; no amplitude discontinuity at block boundaries
- Unit: `tests/dsp/test_true_peak.cpp::test_reset_peak_preserves_fir_state` — settle 100 samples, resetPeak(), process 100 more; peak >= expected true peak (no NaN/corruption)
- Unit: `tests/dsp/test_true_peak.cpp::test_negative_input_tracked_as_absolute` — all-negative samples at -0.8; getPeak() >= 0.8

## Technical Details
7-sample blocks are coprime with typical FIR lengths, maximizing partial-block edge coverage.

## Dependencies
None
