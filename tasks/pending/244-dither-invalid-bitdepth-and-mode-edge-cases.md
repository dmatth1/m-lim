# Task 244: Dither Invalid Bit Depth and Mode Edge Case Tests

## Description
`tests/dsp/test_dither.cpp` covers 16-bit and 24-bit quantization, noise shaping modes, and bit-depth switching, but is missing:

1. **Invalid bit depth boundary test**: No test calls `setBitDepth()` with out-of-range values (0, -1, 33). The source should clamp or ignore invalid values without crashing or producing NaN/Inf output.

2. **Input near ±1.0 boundary**: No test verifies output is bounded when input is exactly 1.0f or -1.0f (at the quantization ceiling). With dither noise added, the output could theoretically exceed 1.0; the test should verify behavior.

3. **Input of 0.0f (silence) with dither**: No test verifies that dithered silence produces non-zero noise (dither is supposed to add low-level noise even to silence) and that noise level is bounded within the expected range (< 1 LSB for the given bit depth).

4. **32-bit bit depth**: No test for `setBitDepth(32)` (the maximum practical depth — at 32-bit quantization, the quantization step is essentially the float epsilon and dither noise should be extremely small).

## Produces
None

## Consumes
None

## Relevant Files
Read: `tests/dsp/test_dither.cpp` — existing tests to extend
Read: `src/dsp/Dither.h` and `src/dsp/Dither.cpp` — to understand clamping behavior and API
Modify: `tests/dsp/test_dither.cpp` — add the new test cases

## Acceptance Criteria
- [ ] Run: `cd /workspace/M-LIM/build && ./tests/MLIMTests "[Dither]" --reporter compact` → Expected: all tests pass

## Tests
- Unit: `tests/dsp/test_dither.cpp::test_invalid_bitdepth_no_nan` — call `setBitDepth(0)`, `setBitDepth(-1)`, `setBitDepth(33)` and process a buffer; verify output is finite (no NaN/Inf)
- Unit: `tests/dsp/test_dither.cpp::test_silence_input_dither_noise_bounded` — set 16-bit depth, process 1024 samples of 0.0f; measure output RMS and verify it is > 0 (dither adds noise) and < 2.0/65536.0 (bounded to 2 LSBs)
- Unit: `tests/dsp/test_dither.cpp::test_input_at_ceiling_no_crash` — process 1024 samples of exactly 1.0f with 24-bit depth, noise shaping mode 1; verify all outputs are finite
- Unit: `tests/dsp/test_dither.cpp::test_32bit_depth_quantization_minimal_noise` — `setBitDepth(32)`, process 1000 samples of 0.5f; verify output deviation from 0.5f is < 1e-6 (essentially no quantization effect at 32-bit)

## Technical Details
- Check source `Dither.cpp` first to determine whether `setBitDepth()` clamps, asserts, or ignores out-of-range values — test should be written to match the actual behavior (clamp = test clamped output; assert = skip or mark as expected behavior in release builds)
- 1 LSB at 16-bit = 1/32768 ≈ 3.05e-5; TPDF dither is bounded to ±1 LSB
- Use `juce::Random` or `std::mt19937` seeded deterministically to avoid flaky noise tests — or use a large enough N and check statistical bounds (RMS) rather than per-sample values

## Dependencies
None
