# Task 531: State Management Cross-Component Integration Tests

## Description
ABState, PresetManager, and UndoManager are tested individually but never together. In PluginProcessor, these three systems interact: preset loading calls `apvts.replaceState()` which bypasses undo tracking; A/B restore also calls `replaceState()`. No test verifies that: (1) undo stack behavior after preset load, (2) A/B state survives preset load/save, (3) preset name persists across A/B toggle, (4) undo after A/B switch. These are realistic user workflows that could silently break.

## Produces
None

## Consumes
None

## Relevant Files
Read: `src/state/ABState.h` — captureState / restoreState via replaceState
Read: `src/state/ABState.cpp` — uses apvts.replaceState()
Read: `src/state/PresetManager.h` — load/save using APVTS
Read: `src/PluginProcessor.h` — owns all three state components
Read: `src/PluginProcessor.cpp` — wiring between state components
Create: `tests/integration/test_state_cross_component.cpp` — new integration test
Modify: `tests/CMakeLists.txt` — add new test file

## Acceptance Criteria
- [ ] Run: `cd build && ctest -R "StateCross" --output-on-failure` → Expected: all tests pass, exit 0

## Tests
- Integration: `tests/integration/test_state_cross_component.cpp::test_preset_load_does_not_corrupt_ab_snapshot` — capture A, load a preset, toggle to A, verify A snapshot is the original captured state (not the loaded preset)
- Integration: `tests/integration/test_state_cross_component.cpp::test_ab_toggle_preserves_preset_name` — load preset "TestPreset", capture A, modify params, toggle back to A, verify preset name is still "TestPreset"
- Integration: `tests/integration/test_state_cross_component.cpp::test_preset_save_after_ab_toggle` — toggle to B (empty), save preset, verify saved preset contains current APVTS state (not stale A state)
- Integration: `tests/integration/test_state_cross_component.cpp::test_undo_after_preset_load` — set param, load preset, attempt undo, verify behavior is defined (undo stack may be cleared or may undo the load)
- Integration: `tests/integration/test_state_cross_component.cpp::test_full_workflow_sequence` — set params → capture A → modify → capture B → save preset → load different preset → toggle A/B → verify each state is self-consistent

## Technical Details
Create a PluginProcessor instance for each test. Use PresetManager with a temp directory. Set parameters via `apvts.getParameter()->setValueNotifyingHost()`. Verify parameter values via `apvts.getRawParameterValue()`.

## Dependencies
None
