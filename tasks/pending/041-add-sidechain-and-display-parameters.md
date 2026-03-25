# Task 041: Add Sidechain Filter and Display Mode Parameters to APVTS

## Description
The SidechainFilter (task 009) exposes setHighPassFreq(), setLowPassFreq(), and setTilt() — but the ParameterLayout interface in SPEC.md defines no corresponding APVTS parameters. Without automatable parameters, the sidechain filter is hardcoded with default values and the user can never adjust it. Pro-L 2 has sidechain filter controls in its UI. Additionally, the waveform display mode (Fast/Slow/SlowDown/Infinite/Off) needs an APVTS parameter so it can be saved/restored with plugin state.

Add four parameters to the APVTS layout and wire the sidechain ones in the PluginProcessor.

## Produces
None

## Consumes
SidechainFilterInterface
ParameterLayout

## Relevant Files
Modify: `M-LIM/src/Parameters.h` — add parameter ID constants
Modify: `M-LIM/src/Parameters.cpp` — add four parameters to createParameterLayout()
Modify: `M-LIM/src/PluginProcessor.cpp` — read sidechain params in processBlock, call setters on LimiterEngine/SidechainFilter
Modify: `SPEC.md` — add parameters to ParameterLayout interface

## Acceptance Criteria
- [ ] Run: `grep -E "sidechainHPFreq|sidechainLPFreq|sidechainTilt|displayMode" M-LIM/src/Parameters.cpp` → Expected: four parameters defined
- [ ] Run: `cd M-LIM && cmake --build build --target MLIM_VST3 -j$(nproc) 2>&1 | tail -3` → Expected: builds successfully

## Tests
- Unit: `tests/integration/test_parameter_state.cpp::test_sidechain_params_exist` — verify the three sidechain parameter IDs exist in APVTS
- Unit: `tests/integration/test_parameter_state.cpp::test_display_mode_parameter_exists` — verify displayMode exists as choice parameter with 5 options

## Technical Details
- Parameter IDs: `"sidechainHPFreq"` (float, 20-2000 Hz, default 20), `"sidechainLPFreq"` (float, 2000-20000 Hz, default 20000), `"sidechainTilt"` (float, -6 to +6 dB, default 0)
- Default values effectively bypass the filter (HP at 20Hz, LP at 20kHz, tilt at 0dB)
- These control the DETECTION path only — they shape what the limiter reacts to, not the audio output
- UI controls for these should be added to the ControlStrip (task 025) or as a separate sidechain section
- `"displayMode"` — choice, 0-4 (Fast/Slow/SlowDown/Infinite/Off), default 0
- SPEC.md ParameterLayout already includes all four parameters — verify implementation matches

## Dependencies
Requires tasks 001, 009
