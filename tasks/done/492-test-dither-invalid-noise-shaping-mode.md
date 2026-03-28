# Task 492: Dither — Invalid Noise Shaping Mode Robustness Tests

## Description
`Dither::setNoiseShaping()` accepts any integer but has no clamping/validation:
modes 0, 1, 2 are defined; modes outside that range silently fall through the
`if (mNoiseShaping == 1) / else if (mNoiseShaping == 2)` chain and produce
unmodified (no-shaping) output. There is no test that:

1. Verifies mode=3, mode=-1, mode=100 behave as "no shaping" (not divergence or NaN)
2. Verifies that switching from an invalid mode back to a valid mode works correctly
3. Documents this behavior so it doesn't become a silent bug if the if-chain is refactored

Without a test, a future refactor (e.g., switching to a switch/case with a default
that zeroes the output) could break behavior silently.

## Produces
None

## Consumes
None

## Relevant Files
Read: `src/dsp/Dither.h` — setNoiseShaping() declaration, mode storage
Read: `src/dsp/Dither.cpp` — if/else chain at lines 100-108 showing fallthrough behavior
Modify: `tests/dsp/test_dither.cpp` — add new test cases

## Acceptance Criteria
- [ ] Run: `cd build && ctest -R "Dither" --output-on-failure` → Expected: all tests pass, exit 0

## Tests
- Unit: `tests/dsp/test_dither.cpp::test_invalid_noise_shaping_mode_finite_output` — setNoiseShaping(3), setNoiseShaping(-1), setNoiseShaping(100); process 1000 samples of sine; all output finite, no divergence
- Unit: `tests/dsp/test_dither.cpp::test_invalid_mode_no_feedback_applied` — with invalid mode, output should match mode=0 (no shaping): compute mean of (output - input) over 1000 silent samples at 16-bit depth; must be within ±2 LSBs
- Unit: `tests/dsp/test_dither.cpp::test_invalid_to_valid_mode_switch_no_diverge` — set mode=5, process 100 samples, then setNoiseShaping(2), process 1000 more; all output finite, no divergence

## Technical Details
Quantization step for 16-bit: `2^(1-16) ≈ 3.05e-5`. Feed a -60 dBFS sine
(amplitude ≈ 0.001) to keep values in range. Divergence would manifest as
output values > 1.0 or NaN. The "no feedback" check: with mode=0, error
feedback is zero, so output = round((input + tpdf) / step) * step; the same
should apply for invalid modes.

## Dependencies
None
