# Task 438: Dither — RNG Reproducibility and Error Feedback Stability at Extreme Rates

## Description
Missing dither tests:
1. Error feedback stability at 88.2/96/192 kHz (long run, no divergence)
2. `reprepare()` flushes prior error state
3. 0.5 LSB boundary quantization is unbiased
4. Correct coefficient branch selected at 88.2 vs. 96 kHz

## Produces
None

## Consumes
None

## Relevant Files
Read: `src/dsp/Dither.h` — noise shaping modes, bit depth, RNG state
Read: `src/dsp/Dither.cpp` — coefficient selection by sample rate
Read: `tests/dsp/test_dither.cpp` — existing coverage
Modify: `tests/dsp/test_dither.cpp` — add new tests

## Acceptance Criteria
- [ ] Run: `cd build && ctest -R "Dither" --output-on-failure` → Expected: all tests pass, exit 0

## Tests
- Unit: `tests/dsp/test_dither.cpp::test_error_feedback_stable_88200hz_long_run` — 100k samples of silence at 88200 Hz; all output finite, within ±2 LSBs
- Unit: `tests/dsp/test_dither.cpp::test_error_feedback_stable_192000hz_long_run` — same at 192000 Hz
- Unit: `tests/dsp/test_dither.cpp::test_reprepare_clears_error_state` — prime error at 44100 Hz, reprepare at 88200, verify output < 2 LSBs
- Unit: `tests/dsp/test_dither.cpp::test_half_lsb_boundary_unbiased` — 10k samples at 0.5 LSB boundary; up/down ratio between 0.4 and 0.6

## Technical Details
Quantization step for 16-bit: `2.0/65536.0 ≈ 3.05e-5`. Feed `0.5 * step` as input repeatedly.
Read `Dither.cpp` coefficient selection before writing to understand 88.2 vs. 96 kHz branches.

## Dependencies
None
