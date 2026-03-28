# Task: LevelingLimiter Multi-Channel Tests (3+ channels)

## Description
LevelingLimiter is tested with 1 and 2 channels only. The implementation uses a per-channel gain array and channel linking, but channels 3-8 are never tested. If the per-channel array has a fixed size or the linking loop has an off-by-one, it would only surface with >2 channels. TransientLimiter has the same gap. Both should be tested with 3, 4, and 6 channels.

## Produces
None

## Consumes
None

## Relevant Files
Read: `src/dsp/LevelingLimiter.h` — channel count handling
Read: `src/dsp/LevelingLimiter.cpp` — per-channel gain array allocation
Read: `src/dsp/TransientLimiter.h` — channel count handling
Read: `src/dsp/TransientLimiter.cpp` — per-channel gain array allocation
Modify: `tests/dsp/test_leveling_limiter.cpp` — add multi-channel tests
Modify: `tests/dsp/test_transient_limiter.cpp` — add multi-channel tests

## Acceptance Criteria
- [ ] Run: `cd build && ctest -R "LevelingLimiter|TransientLimiter" --output-on-failure` → Expected: all tests pass, exit 0

## Tests
- Unit: `tests/dsp/test_leveling_limiter.cpp::test_4_channel_no_crash` — prepare with 4 channels, process loud signal, verify output finite and within bounds
- Unit: `tests/dsp/test_leveling_limiter.cpp::test_6_channel_linking_works` — 6 channels with link=1.0, verify all channels get same gain reduction
- Unit: `tests/dsp/test_transient_limiter.cpp::test_4_channel_peak_limiting` — 4 channels, verify all peaks limited to threshold
- Unit: `tests/dsp/test_transient_limiter.cpp::test_6_channel_linking` — 6 channels with linking, verify linked GR

## Technical Details
Create AudioBuffer with 4 or 6 channels. Fill each channel with different amplitude signals. Process and verify per-channel output.

## Dependencies
None
