# Task: Oversampler — High Factor (16x/32x) and Latency Accuracy Tests

## Description
The existing oversampler tests cover 2x, 4x, and 8x factors, but 16x and 32x are untested. These are valid `setOversamplingFactor()` values (factors 4 and 5 in the 0–5 enum). Additionally:
1. No test verifies that the latency reported by `getLatencyInSamples()` matches the actual measured delay (feed an impulse, measure where it exits downstream).
2. No test verifies that changing oversampling factor mid-stream (via `LimiterEngine::setOversamplingFactor()`) does not leave residual state that causes an output burst or silence gap.
3. No test exercises the 1x (off) → 16x → off round-trip for aliasing regression.

## Produces
None

## Consumes
None

## Relevant Files
Read: `M-LIM/src/dsp/Oversampler.h` — `getLatencyInSamples()`, factor enum
Read: `M-LIM/src/dsp/Oversampler.cpp` — JUCE dsp::Oversampling wrapper
Read: `M-LIM/tests/dsp/test_oversampler.cpp` — existing coverage
Modify: `M-LIM/tests/dsp/test_oversampler.cpp` — add high-factor and latency tests

## Acceptance Criteria
- [ ] Run: `cd build && ctest -R "Oversampler" --output-on-failure` → Expected: all tests pass, exit 0

## Tests
- Unit: `tests/dsp/test_oversampler.cpp::test_16x_no_aliasing` — Prepare at 16x; feed a 440 Hz sine, downsample; output RMS should be within ±1 dB of input RMS.
- Unit: `tests/dsp/test_oversampler.cpp::test_32x_no_aliasing` — Same for 32x factor.
- Unit: `tests/dsp/test_oversampler.cpp::test_reported_latency_matches_measured` — Feed an impulse (sample[0]=1.0, rest 0) through upsample→downsample at 4x. Scan output for peak position; require peak to be within ±2 samples of `getLatencyInSamples()`.
- Unit: `tests/dsp/test_oversampler.cpp::test_factor_change_no_output_burst` — Prepare at 4x, process 10 blocks of sine. Switch to 8x (`prepare()` again). Process 10 more blocks. Verify peak amplitude of first post-switch block is within [-2, +2] dBFS of expected sine peak (no wild burst from stale state).

## Technical Details
- Oversampler factor enum: 0=off, 1=2x, 2=4x, 3=8x, 4=16x, 5=32x (verify in Oversampler.h before writing tests).
- For latency test: block size should be larger than the reported latency so the peak is visible in the output buffer.
- Worker should check if `prepare()` is the right way to change the factor mid-stream (vs. a dedicated setter).

## Dependencies
None
