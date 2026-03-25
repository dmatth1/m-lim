# Task 085: Comprehensive Parameter Layout Tests

## Description
The current `test_parameter_state.cpp` only verifies 4 parameters (3 sidechain + displayMode out of 20+ total). Workers need tests that verify every parameter registered in `createParameterLayout()` is accessible, has correct range/default, and is correctly typed.

Gaps to close:
- All 20 parameters in `ParamID` namespace are accessible via `apvts.getParameter()`
- Float parameters have correct min/max/default values (e.g. inputGain: -12..+36 dB default 0; outputCeiling: -30..0 default 0; lookahead: 0..20 ms; attack: 0.1..50 ms; release: 1..1000 ms)
- Bool parameters exist and have bool type (truePeakEnabled, dcFilterEnabled, ditherEnabled, bypass, unityGainMode, channelLinkTransients, channelLinkRelease, delta)
- Choice parameters have correct number of choices (algorithm: 8, oversamplingFactor: 6, ditherBitDepth: 3, ditherNoiseShaping: 3, displayMode: 5)
- All parameters can be set to min, max, and default without crash
- Setting a parameter value round-trips through normalized→real correctly

## Produces
None

## Consumes
None

## Relevant Files
Read: `src/Parameters.h` — all ParamID constants (20 params)
Read: `src/Parameters.cpp` — actual ranges/defaults to verify against
Modify: `tests/integration/test_parameter_state.cpp` — add comprehensive tests here

## Acceptance Criteria
- [ ] Run: `cd /workspace/M-LIM/build && ctest -R ParameterState --output-on-failure` → Expected: all tests pass, exit 0
- [ ] Run: `grep -c "TEST_CASE" /workspace/M-LIM/tests/integration/test_parameter_state.cpp` → Expected: at least 6 test cases

## Tests
- Unit: `tests/integration/test_parameter_state.cpp::test_all_paramids_accessible` — every ParamID constant returns non-null from apvts.getParameter()
- Unit: `tests/integration/test_parameter_state.cpp::test_float_param_ranges` — inputGain, outputCeiling, lookahead, attack, release have correct min/max/default
- Unit: `tests/integration/test_parameter_state.cpp::test_bool_params_exist` — all 8 bool parameters are accessible and typed correctly
- Unit: `tests/integration/test_parameter_state.cpp::test_choice_param_counts` — algorithm=8, oversamplingFactor=6, ditherBitDepth=3, ditherNoiseShaping=3
- Unit: `tests/integration/test_parameter_state.cpp::test_param_set_to_extremes` — setting each param to min/max/default does not crash

## Technical Details
- Use `dynamic_cast<juce::AudioParameterFloat*>`, `juce::AudioParameterBool*`, `juce::AudioParameterChoice*` to verify types
- Check `param->getNormalisableRange().start`, `.end`, `.defaultValue` for float params
- Read `Parameters.cpp` first to get the exact ranges before writing assertions

## Dependencies
None
