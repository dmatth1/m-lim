# Task: SidechainFilter Extreme Sample Rate Tests

## Description
SidechainFilter is tested at standard rates but not at extreme sample rates (8 kHz, 384 kHz). Filter coefficient calculations using `tan(π * fc / fs)` can produce NaN or unstable poles when fc approaches fs/2. No test verifies stability at very low sample rates where the HP cutoff (up to 2 kHz) could be a significant fraction of Nyquist, or at very high rates where numerical precision of the bilinear transform may degrade.

## Produces
None

## Consumes
None

## Relevant Files
Read: `src/dsp/SidechainFilter.h` — prepare method, coefficient calculation
Read: `src/dsp/SidechainFilter.cpp` — bilinear transform, coefficient clamping
Modify: `tests/dsp/test_sidechain_filter.cpp` — add extreme rate tests

## Acceptance Criteria
- [ ] Run: `cd build && ctest -R "SidechainFilter" --output-on-failure` → Expected: all tests pass, exit 0

## Tests
- Unit: `tests/dsp/test_sidechain_filter.cpp::test_8khz_sample_rate_stable` — prepare at 8000 Hz, set HP to 2000 Hz (near Nyquist), process 1000 samples, verify output is finite and no NaN
- Unit: `tests/dsp/test_sidechain_filter.cpp::test_384khz_sample_rate_stable` — prepare at 384000 Hz, set filters to extreme values, verify stable output
- Unit: `tests/dsp/test_sidechain_filter.cpp::test_hp_near_nyquist_no_nan` — set HP cutoff to just below fs/2, verify no NaN/Inf in output

## Technical Details
The bilinear transform `warp = tan(π * fc / fs)` produces infinity when fc = fs/2. Verify the implementation guards against this edge case.

## Dependencies
None
