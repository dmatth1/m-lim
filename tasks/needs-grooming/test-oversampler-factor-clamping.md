# Task: Oversampler Out-of-Range Factor Clamping Tests

## Description
The Oversampler wraps JUCE's dsp::Oversampling with factors 0-5 (1x through 32x). No test verifies what happens when a factor outside this range is passed (e.g., -1, 6, 100). If the implementation doesn't clamp, this could cause array out-of-bounds or crash. Additionally, no test verifies behavior with a zero-length or single-sample buffer through upsampling/downsampling.

## Produces
None

## Consumes
None

## Relevant Files
Read: `src/dsp/Oversampler.h` — setFactor method, valid range
Read: `src/dsp/Oversampler.cpp` — clamping/validation logic
Modify: `tests/dsp/test_oversampler.cpp` — add out-of-range and edge case tests

## Acceptance Criteria
- [ ] Run: `cd build && ctest -R "Oversampler" --output-on-failure` → Expected: all tests pass, exit 0

## Tests
- Unit: `tests/dsp/test_oversampler.cpp::test_negative_factor_no_crash` — setFactor(-1), prepare, process — verify no crash and factor is clamped to 0
- Unit: `tests/dsp/test_oversampler.cpp::test_factor_above_max_no_crash` — setFactor(6) or setFactor(100), verify clamped to 5
- Unit: `tests/dsp/test_oversampler.cpp::test_single_sample_upsample_downsample` — 1-sample buffer through full up/down cycle, verify finite output
- Unit: `tests/dsp/test_oversampler.cpp::test_zero_sample_buffer_upsample` — 0-sample buffer, verify no crash

## Technical Details
Check the Oversampler implementation for existing clamping. If no clamping exists, the test will expose the bug and the fix should be to add std::clamp in setFactor().

## Dependencies
None
