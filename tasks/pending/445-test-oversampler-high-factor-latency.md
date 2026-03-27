# Task 445: Oversampler — 16x/32x Factors and Latency Accuracy Tests

## Description
Existing oversampler tests cover 2x/4x/8x only. Missing:
1. 16x and 32x no-aliasing tests
2. Reported latency matches actual measured delay (impulse test)
3. Factor change mid-stream produces no output burst

## Produces
None

## Consumes
None

## Relevant Files
Read: `src/dsp/Oversampler.h` — getLatencyInSamples(), factor enum
Read: `src/dsp/Oversampler.cpp` — JUCE dsp::Oversampling wrapper
Read: `tests/dsp/test_oversampler.cpp` — existing coverage
Modify: `tests/dsp/test_oversampler.cpp` — add high-factor and latency tests

## Acceptance Criteria
- [ ] Run: `cd build && ctest -R "Oversampler" --output-on-failure` → Expected: all tests pass, exit 0

## Tests
- Unit: `tests/dsp/test_oversampler.cpp::test_16x_no_aliasing` — 440 Hz sine at 16x; output RMS within ±1 dB of input
- Unit: `tests/dsp/test_oversampler.cpp::test_32x_no_aliasing` — same for 32x
- Unit: `tests/dsp/test_oversampler.cpp::test_reported_latency_matches_measured` — impulse at 4x; peak position within ±2 samples of getLatencyInSamples()
- Unit: `tests/dsp/test_oversampler.cpp::test_factor_change_no_output_burst` — switch 4x→8x; first post-switch block peak within [-2, +2] dBFS of expected

## Technical Details
Factor enum (verify in Oversampler.h): 0=off, 1=2x, 2=4x, 3=8x, 4=16x, 5=32x.
Block size for latency test must exceed reported latency so peak is visible.

## Dependencies
None
