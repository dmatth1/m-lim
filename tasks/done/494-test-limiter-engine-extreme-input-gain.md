# Task 494: LimiterEngine — Extreme Input Gain Boundary Tests

## Description
`LimiterEngine::setInputGain(float dB)` has no range clamping: it converts dB to linear
with `std::pow(10.0f, dB / 20.0f)`. At +120 dB, linearGain = 10^6; at -120 dB,
linearGain ≈ 1e-6. No test verifies that:

1. **Extreme positive gain** (+60 dB, +120 dB): The limiters handle the enormous amplified
   signal without producing NaN/Inf, and output stays bounded at the ceiling.
2. **Extreme negative gain** (-120 dB): Produces near-silence (not silence due to denormals),
   and the ceiling is still valid.
3. **Output ceiling is respected** regardless of how badly the input gain overdrives the chain.

The concern: at +120 dB gain, `buffer.applyGain(1e6f)` on a 0 dBFS signal produces values
of 1e6, which are well within float range (float max ≈ 3.4e38). The TransientLimiter should
clamp these back toward 0 dB, but the sidechain detection operates on these enormous values.
Does the gain reduction logic handle linear values of 1e6 without overflow in intermediate
log/dB computations? Values like `std::log(1e6)` = 13.8, which is fine. But DspUtil::gainToDb
and similar functions should handle this. If there's a bug, output would be NaN or unbounded.

## Produces
None

## Consumes
None

## Relevant Files
Read: `src/dsp/LimiterEngine.cpp` — stepApplyInputGain(), setInputGain(), stepApplyCeiling()
Read: `src/dsp/LimiterEngine.h` — setInputGain() declaration
Read: `src/dsp/DspUtil.h` — gainToDb(), dbToGain(), kDspUtilMinGain
Read: `tests/dsp/test_limiter_engine.cpp` — existing tests to extend
Modify: `tests/dsp/test_limiter_engine.cpp` — add extreme gain tests

## Acceptance Criteria
- [ ] Run: `cd build && ctest -R "LimiterEngine" --output-on-failure` → Expected: all tests pass, exit 0

## Tests
- Unit: `tests/dsp/test_limiter_engine.cpp::test_extreme_positive_gain_output_bounded` — setInputGain(+60.0f), feed 0 dBFS sine, process 10 blocks; all output samples finite, all within [-1.1, 1.1] (ceiling + small tolerance)
- Unit: `tests/dsp/test_limiter_engine.cpp::test_extreme_positive_gain_120db_no_nan` — setInputGain(+120.0f), feed -60 dBFS sine (amplitude 0.001), process 10 blocks; all output finite, none exceed ceiling
- Unit: `tests/dsp/test_limiter_engine.cpp::test_extreme_negative_gain_near_silence` — setInputGain(-120.0f), feed 0 dBFS signal, process 5 blocks; all output finite (no NaN), max abs < 0.001
- Unit: `tests/dsp/test_limiter_engine.cpp::test_input_gain_zero_linear_no_crash` — setInputGain(-∞ equivalent: very small, e.g. -200 dB); process 5 blocks; all output finite

## Technical Details
Prepare at 44100 Hz, 512 samples, 2 channels. Set ceiling to 0 dBFS.
Check all output samples with `std::isfinite()` and `std::abs() <= 1.01f`.
For the -120 dB test, kDspUtilMinGain clamps prevent exact zero (check for this).

## Dependencies
None
