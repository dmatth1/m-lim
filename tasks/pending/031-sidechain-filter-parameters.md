# Task 031: Add Missing Sidechain Filter and Display Mode Parameters

## Description
The ParameterLayout in SPEC.md was missing APVTS parameters for the sidechain filter (HP freq, LP freq, tilt) and waveform display mode. Without these, the sidechain filter and display mode cannot be automated, saved/loaded with state, or connected to UI controls. Add these 4 parameters to `createParameterLayout()` in Parameters.cpp.

## Produces
None

## Consumes
ParameterLayout

## Relevant Files
Modify: `M-LIM/src/Parameters.h` — add parameter ID constants for sidechain and display mode
Modify: `M-LIM/src/Parameters.cpp` — add 4 new parameters to createParameterLayout()
Read: `SPEC.md` — updated ParameterLayout interface with new parameter IDs

## Acceptance Criteria
- [ ] Run: `cd M-LIM && grep -c "sidechainHPFreq\|sidechainLPFreq\|sidechainTilt\|displayMode" src/Parameters.cpp` → Expected: 4 (all four parameter IDs present)
- [ ] Run: `cd M-LIM && cmake --build build --target MLIM_VST3 -j$(nproc) 2>&1 | tail -3` → Expected: builds successfully

## Tests
- Unit: `tests/integration/test_parameter_state.cpp::test_sidechain_parameters_exist` — verify sidechainHPFreq, sidechainLPFreq, sidechainTilt exist in APVTS with correct ranges
- Unit: `tests/integration/test_parameter_state.cpp::test_display_mode_parameter_exists` — verify displayMode exists as choice parameter with 5 options

## Technical Details
- `"sidechainHPFreq"` — float, 20 to 2000 Hz, default 20, skew factor for logarithmic feel
- `"sidechainLPFreq"` — float, 2000 to 20000 Hz, default 20000, skew factor for logarithmic feel
- `"sidechainTilt"` — float, -6 to +6 dB, default 0
- `"displayMode"` — choice, 0-4 (Fast/Slow/SlowDown/Infinite/Off), default 0
- These must also be serialized in state save/load (handled automatically by APVTS)

## Dependencies
Requires task 001
