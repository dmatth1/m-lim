# Task 035: Add Sidechain Filter Parameters to APVTS

## Description
The SidechainFilter (task 009) exposes setHighPassFreq(), setLowPassFreq(), and setTilt() — but the ParameterLayout interface in SPEC.md defines no corresponding APVTS parameters. Without automatable parameters, the sidechain filter is hardcoded with default values and the user can never adjust it. Pro-L 2 has sidechain filter controls in its UI.

Add three parameters to the APVTS layout and wire them in the PluginProcessor.

## Produces
None

## Consumes
SidechainFilterInterface
ParameterLayout

## Relevant Files
Modify: `M-LIM/src/Parameters.h` — add parameter ID constants
Modify: `M-LIM/src/Parameters.cpp` — add three parameters to createParameterLayout()
Modify: `M-LIM/src/PluginProcessor.cpp` — read sidechain params in processBlock, call setters on LimiterEngine/SidechainFilter
Modify: `SPEC.md` — add parameters to ParameterLayout interface

## Acceptance Criteria
- [ ] Run: `grep -E "sidechainHP|sidechainLP|sidechainTilt" M-LIM/src/Parameters.cpp` → Expected: three sidechain parameters defined
- [ ] Run: `cd M-LIM && cmake --build build --target MLIM_VST3 -j$(nproc) 2>&1 | tail -3` → Expected: builds successfully

## Tests
- Unit: `tests/integration/test_parameter_state.cpp::test_sidechain_params_exist` — verify the three parameter IDs exist in APVTS

## Technical Details
- Parameter IDs: `"sidechainHP"` (float, 20-2000 Hz, default 20), `"sidechainLP"` (float, 2000-20000 Hz, default 20000), `"sidechainTilt"` (float, -6 to +6 dB, default 0)
- Default values effectively bypass the filter (HP at 20Hz, LP at 20kHz, tilt at 0dB)
- These control the DETECTION path only — they shape what the limiter reacts to, not the audio output
- UI controls for these should be added to the ControlStrip (task 025) or as a separate sidechain section
- Also update SPEC.md ParameterLayout interface to include these parameters

## Dependencies
Requires tasks 001, 009
