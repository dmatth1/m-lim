# Task 075: Oversampler All Factors and Output Validity Tests

## Description
`test_oversampler.cpp` only explicitly tests factor 1 (2x) and mentions factor 2 (4x) in the deferred test. Factors 3 (8x), 4 (16x), and 5 (32x) have no test coverage. Additionally there are no tests that the output samples are finite/valid after upsample+downsample cycles.

Gaps to close:
- All 6 factors (0–5 = 1x/2x/4x/8x/16x/32x) produce the correct upsampled block size (blockSize * 2^factor)
- After upsample+downsample with a sine input, output samples are all finite (no NaN/Inf)
- Latency increases monotonically with factor (latency(4x) > latency(2x) > latency(0))
- Consecutive upsample/downsample cycles (10 blocks) with any factor don't produce diverging output
- Calling `setFactor()` with the same factor twice is idempotent (no crash, correct behavior)
- Small block sizes (1, 4, 16 samples) with oversampling don't crash or produce wrong-size output

## Produces
None

## Consumes
None

## Relevant Files
Read: `src/dsp/Oversampler.h` — factor mapping: 0=1x, 1=2x, 2=4x, 3=8x, 4=16x, 5=32x
Read: `src/dsp/Oversampler.cpp` — implementation details for latency
Modify: `tests/dsp/test_oversampler.cpp` — add new test cases

## Acceptance Criteria
- [ ] Run: `cd /workspace/M-LIM/build && ctest -R Oversampler --output-on-failure` → Expected: all tests pass, exit 0
- [ ] Run: `grep -c "TEST_CASE" /workspace/M-LIM/tests/dsp/test_oversampler.cpp` → Expected: at least 8 test cases

## Tests
- Unit: `tests/dsp/test_oversampler.cpp::test_all_factors_upsample_size` — for factors 0-5, upsampled block size == blockSize * (1 << factor)
- Unit: `tests/dsp/test_oversampler.cpp::test_output_finite_after_cycle` — for each factor, after upsample+downsample of a sine, all output samples are finite
- Unit: `tests/dsp/test_oversampler.cpp::test_latency_monotonic` — getLatencySamples() increases with factor: 0 < latency(1) < latency(2) < latency(3)
- Unit: `tests/dsp/test_oversampler.cpp::test_repeated_cycles_stable` — 10 cycles of upsample+downsample with factor=2 produce consistent (not growing) output magnitudes
- Unit: `tests/dsp/test_oversampler.cpp::test_small_block_size_no_crash` — factor=2, blockSize=4 processes without crash and returns correct size
- Unit: `tests/dsp/test_oversampler.cpp::test_set_same_factor_idempotent` — calling setFactor(1) twice in a row does not crash and produces correct size

## Technical Details
- For factors 4 (16x) and 5 (32x), use a small block size (64 samples) to avoid test slowness
- Use `std::isfinite()` to check all output samples
- For the latency monotonic test, only compare 4 levels (0, 1, 2, 3) to keep the test fast

## Dependencies
None
