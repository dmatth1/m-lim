# Task 439: DCFilter — High Sample Rate Stability and Edge Case Tests

## Description
Missing DCFilter tests:
1. Stability at 192 kHz (long settling time — DC actually removed within 1 second)
2. Multiple `prepare()` calls correctly flush state
3. Single-sample blocks are no-op for state (same result as bulk)

## Produces
None

## Consumes
None

## Relevant Files
Read: `src/dsp/DCFilter.h` — R coefficient, xPrev, yPrev
Read: `src/dsp/DCFilter.cpp` — prepare() implementation
Modify: `tests/dsp/test_dc_filter.cpp` — add new test cases

## Acceptance Criteria
- [ ] Run: `cd build && ctest -R "DCFilter" --output-on-failure` → Expected: all tests pass, exit 0

## Tests
- Unit: `tests/dsp/test_dc_filter.cpp::test_high_samplerate_dc_removed_192khz` — 192000 samples of DC=0.5f at 192 kHz; DC in latter half < 1e-3
- Unit: `tests/dsp/test_dc_filter.cpp::test_reprepare_clears_state` — prepare(44100), process 1000 DC=1.0 samples, then prepare(48000), process 100 zeros; output[99] within 1e-5 of 0.0
- Unit: `tests/dsp/test_dc_filter.cpp::test_single_sample_blocks` — 44100 single-sample blocks of DC=0.5; accumulated DC in output < 1e-3

## Dependencies
None
