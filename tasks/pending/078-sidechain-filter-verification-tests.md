# Task 078: SidechainFilter Verification — Negative Tilt, Extreme Params, HP>LP Safety

## Description
`test_sidechain_filter.cpp` tests the "happy path" (positive tilt, normal HP/LP values) but is missing:

- Negative tilt should boost lows and attenuate highs (opposite of positive tilt). The test only checks positive tilt values.
- Extreme parameter values (HP=20 Hz, LP=20000 Hz) should not crash and should produce valid output.
- When HP cutoff frequency ≥ LP cutoff frequency (invalid but possible via parameter interaction), the filter must not crash.
- Calling `prepare()` multiple times with different sample rates and then processing must not corrupt state.
- Verify HP filter slope is correct: energy at HP/2 should be at least 6 dB below energy at HP*2 (first-order rolloff check).

## Produces
None

## Consumes
None

## Relevant Files
Read: `src/dsp/SidechainFilter.h` — parameter interface (setHighPassFreq, setLowPassFreq, setTilt)
Read: `src/dsp/SidechainFilter.cpp` — coefficient computation, filter structure
Modify: `tests/dsp/test_sidechain_filter.cpp` — add new test cases

## Acceptance Criteria
- [ ] Run: `cd /workspace/M-LIM/build && ctest -R SidechainFilter --output-on-failure` → Expected: all tests pass, exit 0
- [ ] Run: `grep -c "TEST_CASE" /workspace/M-LIM/tests/dsp/test_sidechain_filter.cpp` → Expected: at least 8 test cases

## Tests
- Unit: `tests/dsp/test_sidechain_filter.cpp::test_negative_tilt_boosts_lows` — with tilt=-6 dB/oct, energy at 100 Hz > energy at 10 kHz (opposite of positive tilt)
- Unit: `tests/dsp/test_sidechain_filter.cpp::test_extreme_params_no_crash` — HP=20 Hz, LP=20000 Hz (full bandwidth), 100 blocks of sine → no crash, finite output
- Unit: `tests/dsp/test_sidechain_filter.cpp::test_hp_above_lp_no_crash` — set HP=10000 Hz, LP=100 Hz, process sine → no crash (behavior may be undefined, but no crash/NaN)
- Unit: `tests/dsp/test_sidechain_filter.cpp::test_reprepare_different_sample_rates` — prepare(44100), process, prepare(96000), process → output finite both times
- Unit: `tests/dsp/test_sidechain_filter.cpp::test_hp_slope_at_least_6db_per_octave` — at HP=1000 Hz: RMS at 500 Hz is at least 6 dB below RMS at 2000 Hz

## Technical Details
- Measure energy by RMS of output after running at least 10 blocks of a pure tone sine
- Use 100 Hz and 10 kHz tones to test tilt direction
- For crash safety tests: just verify no exception is thrown and output samples are finite (`std::isfinite`)

## Dependencies
None
