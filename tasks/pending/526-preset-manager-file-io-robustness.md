# Task 526: Improve PresetManager File I/O Robustness

## Description
`PresetManager::savePreset()` truncates the file before writing (lines 29-31 of `src/state/PresetManager.cpp`). If the process crashes between `truncate()` and `writeTo()`, the preset file is lost. Additionally, `createDirectory()` return values are ignored (lines 7, 13), and preset names are not validated for invalid characters.

## Produces
None

## Consumes
None

## Relevant Files
Modify: `M-LIM/src/state/PresetManager.cpp` — improve savePreset(), add name validation, check createDirectory() results

## Acceptance Criteria
- [ ] Run: `cd build && ctest --output-on-failure -R preset` → Expected: all preset tests pass
- [ ] Run: `grep -n "flush\|flushBuffer" M-LIM/src/state/PresetManager.cpp` → Expected: flush call after writeTo

## Tests
- Unit: `tests/state/test_preset_manager.cpp::test_savePreset_rejects_empty_name` — empty name returns false
- Unit: `tests/state/test_preset_manager.cpp::test_savePreset_rejects_path_separators` — name with `/` or `\` returns false

## Technical Details
1. Add `stream.flush()` after `xml->writeTo(stream)` in savePreset()
2. Validate preset name at top of savePreset(): reject empty, names with `/`, `\`, or `..`
3. Check `createDirectory()` return in constructor and `setPresetDirectory()` — log warning on failure

## Dependencies
None
