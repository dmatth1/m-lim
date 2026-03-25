# Task 033: LimiterEngine Bypass/Delta/Unity Gain Mode Tests

## Description
Task 013 defines bypass, delta, and unity gain modes in the LimiterEngine interface but specifies zero tests for them. These modes affect the entire signal path and are user-facing features. Add dedicated tests for each mode.

## Produces
None

## Consumes
LimiterEngineInterface

## Relevant Files
Create: `M-LIM/tests/dsp/test_limiter_engine_modes.cpp` — mode-specific tests
Read: `M-LIM/src/dsp/LimiterEngine.h` — LimiterEngine interface with mode setters

## Acceptance Criteria
- [ ] Run: `cd M-LIM && cmake --build build --target MLIMTests -j$(nproc) && cd build && ctest -R test_limiter_engine_modes --output-on-failure` → Expected: all tests pass

## Tests
- Unit: `tests/dsp/test_limiter_engine_modes.cpp::test_bypass_passes_signal_unchanged` — enable bypass, process loud signal, verify output equals input (bit-exact or within float epsilon)
- Unit: `tests/dsp/test_limiter_engine_modes.cpp::test_bypass_disables_all_processing` — in bypass, gain reduction should be 0 dB
- Unit: `tests/dsp/test_limiter_engine_modes.cpp::test_delta_outputs_difference` — enable delta mode, process signal that triggers limiting, verify output = input - limited_output (the removed portion)
- Unit: `tests/dsp/test_limiter_engine_modes.cpp::test_delta_silence_when_no_limiting` — delta mode with signal below threshold should output silence
- Unit: `tests/dsp/test_limiter_engine_modes.cpp::test_unity_gain_compensates_ceiling` — unity gain mode should compensate for output ceiling, so input and output loudness match
- Unit: `tests/dsp/test_limiter_engine_modes.cpp::test_mode_switch_no_glitch` — toggle each mode on/off mid-stream without audio glitches (no sudden level jumps > 6dB between consecutive blocks)
- Unit: `tests/dsp/test_limiter_engine_modes.cpp::test_dc_filter_toggle` — enable/disable DC filter during processing, verify no crash
- Unit: `tests/dsp/test_limiter_engine_modes.cpp::test_dither_toggle` — enable/disable dither during processing, verify no crash

## Technical Details
- For bypass test, compare input and output buffers sample-by-sample with tolerance of 1e-6
- For delta test, verify: input = output_normal + output_delta (within tolerance)
- For unity gain test, verify RMS of output ≈ RMS of input when unity gain enabled
- Mode switch test: process 10 blocks toggling mode each block, verify no sample exceeds ±10.0 (sanity bound)

## Dependencies
Requires task 013
