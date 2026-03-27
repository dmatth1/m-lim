# Task 485: PluginProcessor State Restore — Invalid/Corrupt Data Robustness Tests

## Description
`setStateInformation()` only checks `xmlState->hasTagName(apvts.state.getType())` before
restoring state (line 175 in PluginProcessor.cpp). Missing tests for:
1. Zero-length data (sizeInBytes=0) — should not crash
2. Random binary garbage — should not crash or corrupt parameters
3. Valid XML with wrong root tag — should be rejected (already checked, but not tested)
4. Truncated valid state (first 50% of a valid binary) — should not crash
5. Double-restore (call setStateInformation twice rapidly) — no crash

The existing `test_state_save_load` only tests the happy path roundtrip. A DAW can feed
arbitrary data to `setStateInformation` (e.g. from a corrupted project file), so robustness
here is critical.

## Produces
None

## Consumes
None

## Relevant Files
Read: `src/PluginProcessor.cpp` — setStateInformation() (~lines 171-180)
Read: `tests/integration/test_plugin_processor.cpp` — existing state tests
Modify: `tests/integration/test_plugin_processor.cpp` — add robustness tests

## Acceptance Criteria
- [ ] Run: `cd build && ctest -R "PluginProcessor" --output-on-failure` → Expected: all tests pass, exit 0

## Tests
- Integration: `tests/integration/test_plugin_processor.cpp::test_setstate_zero_length_no_crash` — call setStateInformation(nullptr, 0); no crash, parameters unchanged
- Integration: `tests/integration/test_plugin_processor.cpp::test_setstate_random_garbage_no_crash` — 1024 bytes of random data; no crash, parameters unchanged
- Integration: `tests/integration/test_plugin_processor.cpp::test_setstate_wrong_tag_rejected` — valid XML with `<WrongTag>` root; parameters unchanged
- Integration: `tests/integration/test_plugin_processor.cpp::test_setstate_truncated_binary_no_crash` — first half of a valid getStateInformation() output; no crash
- Integration: `tests/integration/test_plugin_processor.cpp::test_setstate_double_restore_no_crash` — two rapid setStateInformation calls; no crash, final state is from second call

## Technical Details
For "parameters unchanged": capture APVTS state before, call setStateInformation with bad
data, verify state matches captured snapshot. For random data: use a deterministic seed
so the test is reproducible.

## Dependencies
None
