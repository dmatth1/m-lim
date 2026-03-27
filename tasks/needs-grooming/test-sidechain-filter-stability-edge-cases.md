# Task: SidechainFilter — Coefficient Stability and State Divergence Tests

## Description
SidechainFilter applies HP, LP, and tilt filtering to the sidechain detection path. The existing tests cover normal parameter ranges, but miss:

1. **Filter state divergence on inverted HP/LP by large margin**: Setting HP = 2000 Hz and LP = 200 Hz (HP > LP by 1800 Hz, an inverted passband) is tested for "no crash" but not for absence of NaN/Inf output after sustained operation (1000+ blocks).

2. **Coefficient update causing state explosion**: When filter coefficients are updated mid-stream (e.g., HP sweeps from 20 Hz to 2000 Hz), the filter state variables (y1, y2) from the old coefficients may resonate with the new coefficients. No test verifies that a rapid parameter sweep doesn't produce output > 100 dBFS.

3. **Re-prepare clears residual state**: After a long session with tilt = +10 dB, calling `prepare()` with a new sample rate should reset the filter state. No test verifies this — residual state could cause a pop/click on the next audio session.

4. **Tilt pivot frequency accuracy**: The tilt filter should have unity gain at 1 kHz (its pivot frequency). No test measures the actual frequency response to verify the pivot is at 1 kHz and not drifted.

## Produces
None

## Consumes
None

## Relevant Files
Read: `M-LIM/src/dsp/SidechainFilter.h` — setHP(), setLP(), setTilt(), prepare()
Read: `M-LIM/src/dsp/SidechainFilter.cpp` — Butterworth coefficient calculation, state variables
Read: `M-LIM/tests/dsp/test_sidechain_filter.cpp` — existing coverage
Modify: `M-LIM/tests/dsp/test_sidechain_filter.cpp` — add edge case tests

## Acceptance Criteria
- [ ] Run: `cd build && ctest -R "SidechainFilter" --output-on-failure` → Expected: all tests pass, exit 0

## Tests
- Unit: `tests/dsp/test_sidechain_filter.cpp::test_inverted_hp_lp_long_run_stable` — Set HP=2000, LP=200. Process 100000 samples of white noise. Verify all output samples are finite and within [-10, +10] (no runaway gain).
- Unit: `tests/dsp/test_sidechain_filter.cpp::test_hp_sweep_no_explosion` — Sweep HP from 20 Hz to 2000 Hz in 1 Hz steps while processing a 100-sample block at each step. Verify peak output across all blocks is < 40 dBFS (no resonance explosion).
- Unit: `tests/dsp/test_sidechain_filter.cpp::test_reprepare_clears_filter_state` — Set tilt=+10 dB, process 10000 samples of loud sine to prime state. Call `prepare(sampleRate)`. Process 10 samples of silence. Verify all 10 output samples are within 1e-4 of 0.0 (state was reset).
- Unit: `tests/dsp/test_sidechain_filter.cpp::test_tilt_unity_at_pivot_frequency` — Set tilt = +6 dB. Measure RMS output at 1 kHz (expected: near input RMS, pivot frequency unchanged). Measure RMS at 100 Hz (expected: lower than input) and 10 kHz (expected: higher than input). Verify 1 kHz attenuation is within ±0.5 dB.

## Technical Details
- For the sweep test, a "100-sample block per step" means creating a new AudioBuffer for each Hz increment. 1980 increments × 100 samples = ~198k samples total.
- Tilt unity at pivot: the test requires knowing the tilt filter's pivot frequency — read SidechainFilter.cpp to confirm it is 1 kHz before writing the expected value.
- State divergence tests are critical for correctness in live performance: a parameter change mid-song must not cause audible artifacts in the sidechain path.

## Dependencies
None
