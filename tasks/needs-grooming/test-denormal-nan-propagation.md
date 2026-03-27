# Task: Test Denormal and NaN Propagation Through DSP Chain

## Description
No tests verify that subnormal (denormal) floats or NaN/Inf values fed into the DSP chain are handled safely. The audio thread has `juce::ScopedNoDenormals` in some places (DCFilter), but TransientLimiter, LevelingLimiter, and TruePeakDetector are untested with denormal inputs. A denormal that survives into the lookahead buffer or FIR state could cause sustained CPU spikes; a NaN could corrupt all downstream output silently.

## Produces
None

## Consumes
None

## Relevant Files
Read: `M-LIM/src/dsp/TransientLimiter.h` — lookahead buffer could accumulate denormals
Read: `M-LIM/src/dsp/LevelingLimiter.h` — release envelope state could go subnormal
Read: `M-LIM/src/dsp/TruePeakDetector.h` — FIR delay line holds past samples
Read: `M-LIM/src/dsp/LimiterEngine.h` — top-level chain, ceiling hard-clip path
Modify: `M-LIM/tests/dsp/test_dsp_edge_cases.cpp` — add tests here (or new file test_denormal_safety.cpp)

## Acceptance Criteria
- [ ] Run: `cd build && ctest -R "denormal|NaN|EdgeCase" --output-on-failure` → Expected: all tests pass, exit 0
- [ ] Run: `cd build && ctest -R "RealtimeSafety" --output-on-failure` → Expected: no regressions

## Tests
- Unit: `tests/dsp/test_dsp_edge_cases.cpp::test_denormal_input_transient_limiter` — Feed a buffer where every sample is `std::numeric_limits<float>::denorm_min()`. After processing, all output samples must be finite and within [-1,1].
- Unit: `tests/dsp/test_dsp_edge_cases.cpp::test_denormal_input_leveling_limiter` — Same as above for LevelingLimiter.
- Unit: `tests/dsp/test_dsp_edge_cases.cpp::test_denormal_survives_multiple_blocks` — Feed 100 blocks of denormals through LimiterEngine; assert envelope state never goes NaN (check meter data `isFinite`).
- Unit: `tests/dsp/test_dsp_edge_cases.cpp::test_nan_input_does_not_corrupt_engine` — Feed one block with a NaN at sample 0. All subsequent blocks of normal sine must produce finite output (engine must not stay permanently corrupted).
- Unit: `tests/dsp/test_dsp_edge_cases.cpp::test_inf_input_clipped_at_ceiling` — Feed +Inf input; output must equal the ceiling value, not Inf/NaN.

## Technical Details
- `std::numeric_limits<float>::denorm_min()` is the smallest positive subnormal (~1.4e-45).
- Use `LimiterEngine` with a simple sine as follow-up to verify recovery after bad input.
- DCFilter already has `juce::ScopedNoDenormals`; the test should verify the engine as a whole.
- Tests can live in `test_dsp_edge_cases.cpp` (already exists) or a new file — worker's choice.

## Dependencies
None
