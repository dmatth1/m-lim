# Task 203: PresetManager — navigation, wrapping, and large-count edge cases

## Description
`test_preset_manager.cpp` and `test_preset_manager_errors.cpp` cover save/load and error paths
but miss the navigation workflow (next/previous preset) and large-count behavior.

Add tests for:

1. **`loadNext()` and `loadPrevious()` wrapping** — if the API exposes `loadNext()` /
   `loadPrevious()`, create 3 presets, load the last one, call `loadNext()`, verify it wraps to
   the first preset. Similarly for `loadPrevious()` on the first preset → wraps to last.
   If the API does not expose `loadNext()`/`loadPrevious()` (check `PresetManager.h`), skip with
   a comment explaining what navigation API exists instead.

2. **Overwrite existing preset** — save preset "Foo", change parameters, save preset "Foo" again;
   load "Foo"; verify the new parameter values are loaded (not the original).

3. **Delete preset** — if `deletePreset()` exists: save preset "TempPreset", delete it, then
   attempt to load "TempPreset"; the load must fail gracefully (return false or error state, not crash).

4. **`getPresetNames()` reflects filesystem** — save three presets, call `getPresetNames()`
   (or equivalent enumeration API), verify all three names appear in the result.

5. **Large number of presets (50 presets)** — create 50 presets in a loop, call the enumeration
   API, verify exactly 50 entries are returned. Then load preset #25 by name and verify the
   parameter values match what was saved.

6. **Preset file with wrong format** — if the manager reads XML, write a file with valid extension
   but invalid XML content; `loadPreset()` must return false or throw a recoverable error, not crash.
   (This may already be covered in `test_preset_manager_errors.cpp` — skip if already present.)

## Produces
None

## Consumes
None

## Relevant Files
Modify: `tests/state/test_preset_manager.cpp` — add new TEST_CASEs for navigation and large count
Read: `tests/state/test_preset_manager_errors.cpp` — check what error cases are already covered
Read: `src/state/PresetManager.h` — full public API (save, load, delete, enumerate, navigate)

## Acceptance Criteria
- [ ] Run: `cd /workspace/M-LIM/build && ctest -R "PresetManager" --output-on-failure` → Expected: all preset manager tests pass, exit 0

## Tests
- Unit: `tests/state/test_preset_manager.cpp::test_load_next_wraps` — loadNext() at last preset wraps to first (or skip if API absent)
- Unit: `tests/state/test_preset_manager.cpp::test_load_previous_wraps` — loadPrevious() at first preset wraps to last (or skip if API absent)
- Unit: `tests/state/test_preset_manager.cpp::test_overwrite_preset` — saving same name twice updates values on load
- Unit: `tests/state/test_preset_manager.cpp::test_delete_preset` — deleted preset cannot be loaded (or skip if API absent)
- Unit: `tests/state/test_preset_manager.cpp::test_get_preset_names_reflects_saves` — three saves appear in enumeration result
- Unit: `tests/state/test_preset_manager.cpp::test_fifty_preset_roundtrip` — 50 presets, correct count, named load returns correct values
- Unit: `tests/state/test_preset_manager.cpp::test_malformed_preset_file` — invalid XML file handled gracefully (skip if already in errors file)

## Technical Details
- Check `PresetManager.h` carefully before writing tests — adapt to the actual API surface.
  The navigation API may be `loadPresetByIndex(int)`, `getCurrentPresetIndex()`, etc.
- Use a temporary directory (from the existing test fixture pattern in `test_preset_manager.cpp`)
  for the 50-preset test to avoid polluting the filesystem.
- For the large-count test: save presets named "Preset001" through "Preset050" with a specific
  parameter value (e.g., gain = index as float). After saving all, load "Preset025" and verify
  its gain value equals 25.0f.
- If `juce::TemporaryFile` or `juce::UnitTestRunner` is used in existing tests, follow the same pattern.

## Dependencies
None
