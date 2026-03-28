# Task: PluginProcessor::parameterChanged() Direct Test Coverage

## Description
`PluginProcessor::parameterChanged()` (line ~258) handles parameter change callbacks from APVTS. It sets `mParametersDirty`, and has special handling for `oversamplingFactor` (triggers async update) and `lookahead` (calls `limiterEngine.setLookahead` + `updateLatency`). This method is only tested indirectly via higher-level tests. A direct test should verify: (1) mParametersDirty is set after any parameter change, (2) lookahead changes update latency immediately, (3) oversampling changes trigger async rebuild, (4) non-special parameters don't trigger async update or latency change.

## Produces
None

## Consumes
None

## Relevant Files
Read: `src/PluginProcessor.cpp` — parameterChanged() at ~line 258
Read: `src/PluginProcessor.h` — mParametersDirty, mOversamplingChangePending atomics
Modify: `tests/integration/test_plugin_processor.cpp` — add tests

## Acceptance Criteria
- [ ] Run: `cd build && ctest -R "PluginProcessor" --output-on-failure` → Expected: all tests pass, exit 0

## Tests
- Integration: `tests/integration/test_plugin_processor.cpp::test_parameter_change_sets_dirty_flag` — change a parameter via APVTS, verify some observable effect (e.g., next processBlock picks up the new value)
- Integration: `tests/integration/test_plugin_processor.cpp::test_lookahead_change_updates_latency_immediately` — set lookahead to 5ms, check getLatencySamples(); change to 1ms, check latency decreased without calling prepareToPlay
- Integration: `tests/integration/test_plugin_processor.cpp::test_oversampling_change_triggers_pending_flag` — change oversamplingFactor param, verify that after message thread processes, latency changes

## Technical Details
The dirty flag and pending oversampling flag are private atomics, so testing must be done through observable side effects: latency changes, processBlock output differences after param change, etc.

## Dependencies
None
