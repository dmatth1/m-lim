# Task 440: Test Denormal and NaN/Inf Propagation Through DSP Chain

## Description
No tests verify that subnormal (denormal) floats or NaN/Inf values fed into the DSP
chain are handled safely. Denormals in lookahead buffers can cause CPU spikes; NaN can
corrupt all downstream output silently.

## Produces
None

## Consumes
None

## Relevant Files
Read: `src/dsp/TransientLimiter.h` — lookahead buffer
Read: `src/dsp/LevelingLimiter.h` — release envelope state
Read: `src/dsp/TruePeakDetector.h` — FIR delay line
Read: `src/dsp/LimiterEngine.h` — top-level chain with ceiling hard-clip
Modify: `tests/dsp/test_dsp_edge_cases.cpp` — add tests (or new test_denormal_safety.cpp)

## Acceptance Criteria
- [ ] Run: `cd build && ctest -R "denormal|NaN|EdgeCase" --output-on-failure` → Expected: all tests pass, exit 0

## Tests
- Unit: `tests/dsp/test_dsp_edge_cases.cpp::test_denormal_input_transient_limiter` — buffer of `denorm_min()` inputs; all output finite within [-1,1]
- Unit: `tests/dsp/test_dsp_edge_cases.cpp::test_denormal_input_leveling_limiter` — same for LevelingLimiter
- Unit: `tests/dsp/test_dsp_edge_cases.cpp::test_denormal_survives_multiple_blocks` — 100 blocks of denormals through LimiterEngine; meter data stays finite
- Unit: `tests/dsp/test_dsp_edge_cases.cpp::test_nan_input_does_not_corrupt_engine` — one NaN block; subsequent sine blocks produce finite output
- Unit: `tests/dsp/test_dsp_edge_cases.cpp::test_inf_input_clipped_at_ceiling` — +Inf input; output equals ceiling, not Inf/NaN

## Technical Details
Use `std::numeric_limits<float>::denorm_min()` (~1.4e-45). Worker can use existing
`test_dsp_edge_cases.cpp` or create a new file.

## Dependencies
None
