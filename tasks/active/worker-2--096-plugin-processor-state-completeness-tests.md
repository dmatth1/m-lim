# Task 096: Plugin Processor State Save/Load Completeness Tests

## Description
`test_plugin_processor.cpp` only tests 3 of 20+ parameters in the state save/load round-trip. If any other parameter silently fails to serialize, there is no test catching it. Also missing: bus layout support test, tail length test, and corrupted state safety.

Gaps to close:
- State save/load round-trip for all 20 parameters (not just inputGain, outputCeiling, lookahead)
- `isBusesLayoutSupported()` returns true for stereo in/out and false for mono
- `getTailLengthSeconds()` returns a positive value (due to lookahead + release time)
- `setStateInformation()` with empty/null/garbage data does not crash
- `setStateInformation()` with a truncated but valid XML snippet does not crash
- `acceptsMidi()` returns false

## Produces
None

## Consumes
None

## Relevant Files
Read: `src/PluginProcessor.h` — processor interface
Read: `src/PluginProcessor.cpp` — getStateInformation / setStateInformation implementation
Read: `src/Parameters.h` — all 20 ParamID constants
Modify: `tests/integration/test_plugin_processor.cpp` — add tests

## Acceptance Criteria
- [ ] Run: `cd /workspace/M-LIM/build && ctest -R PluginProcessor --output-on-failure` → Expected: all tests pass, exit 0
- [ ] Run: `grep -c "TEST_CASE" /workspace/M-LIM/tests/integration/test_plugin_processor.cpp` → Expected: at least 8 test cases

## Tests
- Integration: `tests/integration/test_plugin_processor.cpp::test_state_round_trip_all_params` — set all 20 parameters to non-default values, save state, create new processor, load state, verify all params match
- Integration: `tests/integration/test_plugin_processor.cpp::test_bus_layout_stereo_supported` — `isBusesLayoutSupported(stereo in/out)` returns true
- Integration: `tests/integration/test_plugin_processor.cpp::test_tail_length_positive` — after `prepareToPlay()`, `getTailLengthSeconds()` > 0
- Integration: `tests/integration/test_plugin_processor.cpp::test_set_state_empty_no_crash` — `setStateInformation(nullptr, 0)` does not crash
- Integration: `tests/integration/test_plugin_processor.cpp::test_set_state_garbage_no_crash` — `setStateInformation` with random bytes does not crash
- Integration: `tests/integration/test_plugin_processor.cpp::test_accepts_midi_false` — `acceptsMidi()` returns false

## Technical Details
- For the round-trip test: iterate all ParamIDs, use `apvts.getRawParameterValue(id)` to set and verify values
- For the bus layout test: construct `BusesLayout` with stereo input and stereo output
- To create garbage data: use `std::vector<char>` with 64 random bytes

## Dependencies
None
