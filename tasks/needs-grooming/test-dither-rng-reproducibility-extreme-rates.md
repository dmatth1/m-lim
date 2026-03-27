# Task: Dither — RNG Reproducibility and Error Feedback Stability at Extreme Sample Rates

## Description
The Dither module uses a TPDF (Triangular PDF) random number generator for dither noise. Currently untested:

1. **RNG reproducibility**: No test verifies that given the same seed (if supported) or a reset state, the dither sequence is reproducible. This matters for automated test stability — if the dither noise is truly random, test assertions on exact sample values will be flaky.

2. **Error feedback stability at 88.2/96/192 kHz**: The existing test `test_high_samplerate_fallback` verifies that at >= 88.2 kHz, feedback coefficients are zeroed. But there is no test that feeds a long run (e.g., 100k samples) at 88.2 kHz to verify the filter state does not diverge (feedback of 0 should produce a trivially stable filter, but residual state from a prior 44.1 kHz session must be cleared on `prepare()`).

3. **Quantization rounding boundary at exactly 0.5 LSB**: Input of exactly N + 0.5 LSB (where N is an integer multiple of the quantization step) is a rounding boundary. The dither should randomize which way this rounds — over many samples, half should round up and half down. No test verifies this probability distribution.

4. **Noise shaping coefficients are sample-rate-specific**: Tests exist for 44.1 kHz and 48 kHz, but no test verifies the correct coefficient branch is selected at 88.2 kHz vs. 96 kHz.

## Produces
None

## Consumes
None

## Relevant Files
Read: `M-LIM/src/dsp/Dither.h` — noise shaping modes, bit depth, RNG state
Read: `M-LIM/src/dsp/Dither.cpp` — coefficient selection by sample rate, RNG implementation
Read: `M-LIM/tests/dsp/test_dither.cpp` — existing coverage
Modify: `M-LIM/tests/dsp/test_dither.cpp` — add new tests

## Acceptance Criteria
- [ ] Run: `cd build && ctest -R "Dither" --output-on-failure` → Expected: all tests pass, exit 0

## Tests
- Unit: `tests/dsp/test_dither.cpp::test_error_feedback_stable_88200hz_long_run` — Prepare at 88200 Hz, 16-bit, noise shaping mode 1. Feed 100000 samples of silence. Verify all output samples are finite and within [-2 LSBs, +2 LSBs] of 0 (no divergence, no explosion).
- Unit: `tests/dsp/test_dither.cpp::test_error_feedback_stable_192000hz_long_run` — Same at 192000 Hz.
- Unit: `tests/dsp/test_dither.cpp::test_reprepare_clears_error_state` — Prepare at 44100 Hz, process 1000 samples of loud signal (to prime error state). Call `prepare(88200, ...)`. Feed 100 samples of silence. Verify output magnitude < 2 LSBs (residual error from 44.1 kHz session was flushed).
- Unit: `tests/dsp/test_dither.cpp::test_half_lsb_boundary_unbiased` — Feed 10000 samples each exactly at the 0.5 LSB rounding boundary for 16-bit (amplitude = 0.5/32768.0). After quantization, count how many round up vs. round down. Require ratio between 0.4 and 0.6 (unbiased, not systematically rounding one direction).

## Technical Details
- 88200 Hz vs. 96000 Hz coefficient selection: read Dither.cpp to find which branch handles these rates before writing tests. If the implementation uses a single "high sample rate" fallback for both, a single test covers both.
- For the 0.5 LSB boundary test: quantization step for 16-bit is `2.0 / 65536.0 ≈ 3.05e-5`. The 0.5 LSB value is half of that. Subtract the un-dithered quantized value to determine which way each sample rounded.

## Dependencies
None
