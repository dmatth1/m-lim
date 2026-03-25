# Task 029: Parameter State Integration Tests

## Description
Write comprehensive integration tests verifying parameter state management: APVTS creation, parameter ranges, state serialization/deserialization roundtrip, A/B state switching, and undo/redo with APVTS.

## Produces
None

## Consumes
ParameterLayout
ABStateInterface
UndoManagerInterface
PluginProcessorCore

## Relevant Files
Create: `M-LIM/tests/integration/test_parameter_state.cpp` — integration tests
Read: `M-LIM/src/Parameters.h` — parameter definitions
Read: `M-LIM/src/state/ABState.h` — A/B state
Read: `M-LIM/src/state/UndoManager.h` — undo manager
Read: `M-LIM/src/PluginProcessor.h` — processor with APVTS

## Acceptance Criteria
- [ ] Run: `cd M-LIM && cmake --build build --target MLIMTests -j$(nproc) && cd build && ctest -R test_parameter_state --output-on-failure` → Expected: all tests pass

## Tests
- Integration: `tests/integration/test_parameter_state.cpp::test_all_parameters_exist` — verify all parameter IDs from SPEC exist in APVTS
- Integration: `tests/integration/test_parameter_state.cpp::test_parameter_ranges` — verify each parameter's min/max/default matches SPEC
- Integration: `tests/integration/test_parameter_state.cpp::test_state_roundtrip` — serialize state, deserialize, verify all params match
- Integration: `tests/integration/test_parameter_state.cpp::test_ab_with_apvts` — A/B toggle correctly swaps parameter values
- Integration: `tests/integration/test_parameter_state.cpp::test_undo_redo_with_apvts` — parameter change can be undone/redone

## Technical Details
- Create a standalone APVTS instance for testing (no need for full processor)
- Test each parameter ID exists: inputGain, outputCeiling, algorithm, lookahead, attack, release, channelLinkTransients, channelLinkRelease, truePeakEnabled, oversamplingFactor, dcFilterEnabled, ditherEnabled, ditherBitDepth, ditherNoiseShaping, bypass, unityGainMode, delta, sidechainHPFreq, sidechainLPFreq, sidechainTilt, displayMode
- Verify AudioProcessor can be instantiated and destroyed without crash
- Test state save/load produces identical parameter values

## Dependencies
Requires tasks 014, 016, 017
