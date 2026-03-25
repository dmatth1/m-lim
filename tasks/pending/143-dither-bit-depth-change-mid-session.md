# Task 143: Dither — Missing Test for Bit Depth Change Mid-Session

## Description
`Dither::setBitDepth()` can be called at any time to change the quantization step size.
The class maintains error-feedback state (`mError1`, `mError2`) that was accumulated
at the previous bit depth. When the bit depth changes, the stored errors may be many
times larger or smaller than the new step size, causing:

- A transient burst of incorrectly shaped noise when the error state from a fine-grained
  depth (e.g., 24-bit) is applied to a coarse depth (e.g., 16-bit), or vice versa.
- In worst case, if the error is orders of magnitude larger than the new step, the feedback
  loop could produce a runaway burst of large values (especially with mode 2 at 44.1 kHz).

No test currently changes `setBitDepth()` mid-stream on an active Dither instance.
The existing `test_16bit_quantization` and `test_24bit_quantization` always create a fresh
`Dither` instance for each test.

## Produces
None

## Consumes
None

## Relevant Files
Read: `M-LIM/src/dsp/Dither.h` — setBitDepth(), mError1, mError2, mStepSize
Read: `M-LIM/src/dsp/Dither.cpp` — how setBitDepth() updates mStepSize; does it reset errors?
Modify: `M-LIM/tests/dsp/test_dither.cpp` — add new test cases

## Acceptance Criteria
- [ ] Run: `cd /workspace/M-LIM/build && ctest -R Dither --output-on-failure` → Expected: all tests pass, exit 0

## Tests
- Unit: `tests/dsp/test_dither.cpp::test_bit_depth_change_16_to_24_no_nan` — prepare at
  44100 Hz with mode 1 (Optimized), process 1000 samples at 16-bit, call `setBitDepth(24)`,
  process 1000 more samples; assert all output samples are finite (no NaN/Inf) and within
  [-1, 1] range.
- Unit: `tests/dsp/test_dither.cpp::test_bit_depth_change_24_to_16_no_nan` — same as above
  but reversed: 24-bit → 16-bit mid-stream. Assert all output is finite.
- Unit: `tests/dsp/test_dither.cpp::test_bit_depth_change_quantization_correct_after` —
  prepare at 44100 Hz, process 512 samples at 16-bit, call `setBitDepth(24)`, process 512
  more samples of a ramp signal, and verify the *final* 512 samples are quantized to 24-bit
  step boundaries (using the isQuantized() helper already in test_dither.cpp).

## Technical Details
- If `setBitDepth()` currently does not reset `mError1`/`mError2`, the fix is to add
  `mError1 = mError2 = 0.0f;` in that method. The tests will detect this bug and the worker
  should fix it if found.
- Use `mode=1` (Optimized, first-order) for the finite-check tests since it has only one
  error term and is easier to reason about; use `mode=0` (Basic) for the quantization
  verification to avoid error-feedback confounding the step-alignment check.

## Dependencies
None
