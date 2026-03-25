# Task 082: PresetManager Overwrite, Special Characters, and Edge Case Tests

## Description
`test_preset_manager.cpp` and `test_preset_manager_errors.cpp` cover basic save/load/navigate and error paths, but miss:

- Saving a preset with the same name twice should overwrite (not create a duplicate entry)
- Preset names with special characters that are illegal in filenames (e.g. `/`, `\`, `:`, `*`, `?`, `"`, `<`, `>`, `|`) should either sanitize the name or handle gracefully without crash
- Very long preset names (255+ characters) should not crash
- Saving and loading preserves the complete parameter state (not just that no crash occurs)
- After deleting all presets, `getPresetNames()` returns an empty list and `next()`/`previous()` don't crash

## Produces
None

## Consumes
None

## Relevant Files
Read: `src/state/PresetManager.h` — savePreset(), loadPreset(), getPresetNames(), next(), previous() interface
Read: `src/state/PresetManager.cpp` — filename handling, XML serialization
Modify: `tests/state/test_preset_manager.cpp` — add new test cases

## Acceptance Criteria
- [ ] Run: `cd /workspace/M-LIM/build && ctest -R PresetManager --output-on-failure` → Expected: all tests pass, exit 0
- [ ] Run: `grep -c "TEST_CASE" /workspace/M-LIM/tests/state/test_preset_manager.cpp` → Expected: at least 8 test cases

## Tests
- Unit: `tests/state/test_preset_manager.cpp::test_save_overwrite_no_duplicate` — save "MyPreset", save "MyPreset" again with different params; `getPresetNames()` returns exactly 1 entry named "MyPreset", not 2
- Unit: `tests/state/test_preset_manager.cpp::test_special_chars_no_crash` — `savePreset("Test/Preset:Name")` does not crash and either succeeds (sanitized) or returns an error (graceful)
- Unit: `tests/state/test_preset_manager.cpp::test_long_name_no_crash` — `savePreset(string(300, 'A'))` does not crash
- Unit: `tests/state/test_preset_manager.cpp::test_save_load_parameter_fidelity` — set inputGain to a non-default value, save preset, reset inputGain, load preset, verify inputGain is restored to the saved value
- Unit: `tests/state/test_preset_manager.cpp::test_empty_preset_list_navigate_no_crash` — when no presets exist, calling `next()` and `previous()` does not crash

## Technical Details
- For overwrite test: use a temporary directory to avoid test pollution
- `savePreset()` may return bool indicating success — check that interface before writing the test
- File fidelity test should use `apvts.getRawParameterValue(ParamID::inputGain)` to verify

## Dependencies
None
