# Task 441: SidechainFilter — Coefficient Stability and State Divergence Tests

## Description
Missing SidechainFilter tests for edge cases that matter in live performance:
1. Inverted HP/LP (HP > LP) long-run stability — no NaN/Inf after 100k blocks
2. HP sweep mid-stream — no resonance explosion
3. Re-prepare clears filter state — no pop/click on new session
4. Tilt unity at 1 kHz pivot frequency

## Produces
None

## Consumes
None

## Relevant Files
Read: `src/dsp/SidechainFilter.h` — setHP(), setLP(), setTilt(), prepare()
Read: `src/dsp/SidechainFilter.cpp` — coefficient calculation, state variables
Read: `tests/dsp/test_sidechain_filter.cpp` — existing coverage
Modify: `tests/dsp/test_sidechain_filter.cpp` — add edge case tests

## Acceptance Criteria
- [ ] Run: `cd build && ctest -R "SidechainFilter" --output-on-failure` → Expected: all tests pass, exit 0

## Tests
- Unit: `tests/dsp/test_sidechain_filter.cpp::test_inverted_hp_lp_long_run_stable` — HP=2000, LP=200, 100k samples white noise; all output finite within [-10, +10]
- Unit: `tests/dsp/test_sidechain_filter.cpp::test_hp_sweep_no_explosion` — HP sweeps 20→2000 Hz, 100 samples per step; peak output < 40 dBFS
- Unit: `tests/dsp/test_sidechain_filter.cpp::test_reprepare_clears_filter_state` — prime with tilt=+10dB + 10k loud samples, reprepare, process 10 silence samples; output within 1e-4 of 0.0
- Unit: `tests/dsp/test_sidechain_filter.cpp::test_tilt_unity_at_pivot_frequency` — at 1 kHz with tilt=+6 dB, attenuation within ±0.5 dB of input

## Dependencies
None
