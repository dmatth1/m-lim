# Task: Dither Intermediate Bit Depth Tests (18/20/22-bit)

## Description
Dither tests cover 16-bit, 24-bit, and 32-bit quantization, plus noise shaping stability and edge cases. However, 18-bit, 20-bit, and 22-bit depths — which are valid settings exposed via the `ditherBitDepth` parameter — have no quantization correctness tests. The quantization step size and noise floor should be verified for these intermediate depths to ensure the formula `step = 1 / (2^(N-1) - 1)` works correctly for all valid N.

## Produces
None

## Consumes
None

## Relevant Files
Read: `src/dsp/Dither.h` — setBitDepth method and step calculation
Read: `src/dsp/Dither.cpp` — process() quantization logic
Modify: `tests/dsp/test_dither.cpp` — add tests for 18/20/22-bit

## Acceptance Criteria
- [ ] Run: `cd build && ctest -R "Dither" --output-on-failure` → Expected: all tests pass, exit 0

## Tests
- Unit: `tests/dsp/test_dither.cpp::test_18bit_quantization_step` — process a known signal with 18-bit dither, verify output quantization step matches expected 1/(2^17 - 1)
- Unit: `tests/dsp/test_dither.cpp::test_20bit_quantization_step` — same for 20-bit
- Unit: `tests/dsp/test_dither.cpp::test_22bit_quantization_step` — same for 22-bit
- Unit: `tests/dsp/test_dither.cpp::test_intermediate_depths_noise_bounded` — verify dither noise amplitude is proportional to quantization step for each depth

## Technical Details
Use a DC signal at 0.5f, process ~1000 samples, verify the output values cluster around the expected quantized levels. Noise floor should be within ±2 LSBs of the quantization step.

## Dependencies
None
