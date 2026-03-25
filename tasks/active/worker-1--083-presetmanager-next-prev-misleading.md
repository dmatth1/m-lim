# Task 083: PresetManager::nextPreset/previousPreset Don't Load the Preset

## Description
`PresetManager::nextPreset()` and `previousPreset()` update `currentPresetName`
by advancing the index in the sorted preset list, but they **do not load** the
selected preset into the APVTS. The caller must call `loadPreset(getCurrentPresetName(), apvts)`
afterward — but this is not documented and the method names strongly imply the
load is included.

This API is confusing in two ways:
1. Method names `nextPreset()`/`previousPreset()` imply the preset changes;
   actually only the cursor moves.
2. These methods don't accept an APVTS reference, so they *cannot* load — but
   there is no warning in the API doc.

Additionally, `nextPreset()` and `previousPreset()` each call `getPresetNames()`
internally, which performs a filesystem scan (`findChildFiles` with recursive
search). If the caller calls `nextPreset()` and then immediately calls
`loadPreset(getCurrentPresetName(), apvts)`, two filesystem scans occur for
a single user action.

**Fix**: Make the intent explicit by renaming the methods to `advanceToNext()`
and `advanceToPrevious()`, or by accepting an optional APVTS pointer and loading
when provided. The recommended approach is to update the method docs with a
clear note that the caller must call `loadPreset` after advancing.

Alternatively (simpler and cleaner): merge the advance-and-load into a single
`loadNextPreset(apvts)` / `loadPreviousPreset(apvts)` API that does both steps
atomically, and deprecate the bare `nextPreset()`/`previousPreset()` methods.

## Produces
None

## Consumes
None

## Relevant Files
Modify: `M-LIM/src/state/PresetManager.h` — rename or add documentation to
  `nextPreset()`/`previousPreset()` OR add `loadNextPreset(apvts)` /
  `loadPreviousPreset(apvts)` wrapper methods.
Modify: `M-LIM/src/state/PresetManager.cpp` — implement the changes.
Read: `M-LIM/tests/state/test_preset_manager.cpp` — check for existing callers
  and update accordingly.
Read: `M-LIM/tests/state/test_preset_manager_errors.cpp` — check for existing callers.

## Acceptance Criteria
- [ ] Run: `cd M-LIM && ctest --test-dir build -R test_preset_manager --output-on-failure` → Expected: all tests pass.
- [ ] Run: `grep -n "nextPreset\|previousPreset" M-LIM/src/state/PresetManager.h` → Expected: method signature or doc clearly indicates whether it loads or only advances.

## Tests
- Unit: `M-LIM/tests/state/test_preset_manager.cpp` — add a test `test_load_next_advances_and_loads` that verifies calling `loadNextPreset(apvts)` (or the equivalent) both advances the name AND restores the correct APVTS state from the preset file.
- Unit: existing tests should all pass after rename/refactor.

## Technical Details
The recommended fix:
1. Add `bool loadNextPreset(juce::AudioProcessorValueTreeState& apvts)` and
   `bool loadPreviousPreset(juce::AudioProcessorValueTreeState& apvts)` — these
   call `getPresetNames()` once, advance the index, call `loadPreset`, and return
   the load result.
2. Keep `nextPreset()`/`previousPreset()` as internal helpers but make them
   `private`, forcing callers to use the combined load methods.

## Dependencies
None
