# Task 149: PresetManager — No Test for Saving Over an Existing Preset

## Description
`PresetManager::savePreset()` should overwrite an existing preset file when called with the
same name a second time. No test currently verifies this "save-overwrite" scenario:

- `test_save_and_load` only saves once and loads.
- `test_save_multiple_presets` saves multiple distinctly-named presets, not the same name twice.

Bugs that would hide without this test:
1. `savePreset("X", apvts)` the second time creates `X(1).mlim` or appends a suffix rather
   than overwriting `X.mlim` — the second load would return the original values.
2. The preset index (used by `loadNextPreset` / `loadPreviousPreset`) counts duplicated
   filenames, causing navigation to visit the "same" preset twice.
3. `getCurrentPresetName()` returns the old name after a re-save with a different name.

## Produces
None

## Consumes
None

## Relevant Files
Read: `M-LIM/src/state/PresetManager.h` — savePreset(), loadPreset(), getCurrentPresetName()
Read: `M-LIM/src/state/PresetManager.cpp` — file write logic; does it overwrite or rename?
Modify: `M-LIM/tests/state/test_preset_manager.cpp` — add overwrite test cases

## Acceptance Criteria
- [ ] Run: `cd /workspace/M-LIM/build && ctest -R PresetManager --output-on-failure` → Expected: all tests pass, exit 0

## Tests
- Unit: `tests/state/test_preset_manager.cpp::test_save_overwrites_existing` — save preset
  "Alpha" with gain=-6, then change gain to -12 and save "Alpha" again; load "Alpha" and
  verify gain=-12 (not -6). Also verify `getPresetNames()` returns only one entry named
  "Alpha" (no duplicate files).
- Unit: `tests/state/test_preset_manager.cpp::test_overwrite_does_not_duplicate_index` —
  save "Beta" three times with different values; call `getPresetNames()` and verify the list
  contains "Beta" exactly once; use `loadNextPreset()` from "Beta" to verify navigation
  skips no extra "Beta" entries.

## Technical Details
- Use the `TempPresetDir` RAII helper already defined in `test_preset_manager.cpp` for
  filesystem isolation.
- Check both the return value of `savePreset()` (should be true on overwrite) and the file
  count in the directory (should remain 1 after saving the same name twice).
- The test should also verify that `getCurrentPresetName()` returns "Alpha" / "Beta" after
  the overwrite, confirming the name tracking is consistent with the file state.

## Dependencies
None
