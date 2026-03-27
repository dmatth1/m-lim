# Task: PresetManager — Concurrent Access and File Corruption Recovery Tests

## Description
PresetManager reads and writes XML files to disk. The existing error tests (`test_preset_manager_errors.cpp`) cover load-nonexistent and malformed-XML, but miss several real-world failure modes:

1. **Concurrent save/load**: No test verifies that simultaneous save and load (from different threads) doesn't corrupt the preset file or the in-memory state. This is particularly relevant when a DAW host may trigger a state save while the user is actively navigating presets.

2. **Partial write recovery**: If the process crashes mid-write, the preset file may be partially written (truncated XML). On next load, this should fail gracefully (return false, not crash) — the current malformed-XML test uses a fully formed but semantically invalid file, not a truncated one.

3. **Unicode and whitespace-only preset names**: A name containing only whitespace (`"   "`) or Unicode characters (`"プリセット"`) may fail to save on some filesystems. Tests should cover both.

4. **Preset count consistency after failed save**: If `savePreset()` fails (unwritable directory), `getPresetNames()` should not include the failed preset. The index should remain unchanged.

## Produces
None

## Consumes
None

## Relevant Files
Read: `M-LIM/src/state/PresetManager.h` — save/load/navigate API
Read: `M-LIM/src/state/PresetManager.cpp` — file I/O implementation
Read: `M-LIM/tests/state/test_preset_manager.cpp` — happy path coverage
Read: `M-LIM/tests/state/test_preset_manager_errors.cpp` — existing error coverage
Modify: `M-LIM/tests/state/test_preset_manager_errors.cpp` — add new error/edge cases

## Acceptance Criteria
- [ ] Run: `cd build && ctest -R "PresetManager" --output-on-failure` → Expected: all tests pass, exit 0

## Tests
- Unit: `tests/state/test_preset_manager_errors.cpp::test_truncated_xml_load_returns_false` — Write a truncated XML file (first 20 bytes of a valid preset). Call `loadPreset(name)`. Require return value is false and no crash.
- Unit: `tests/state/test_preset_manager_errors.cpp::test_failed_save_does_not_add_to_index` — Set preset dir to a read-only path. Call `savePreset("Ghost")`. Verify `getPresetNames()` does not contain `"Ghost"`.
- Unit: `tests/state/test_preset_manager_errors.cpp::test_whitespace_only_name_no_crash` — Call `savePreset("   ")`. Must not crash. Optionally assert return value (false is acceptable if names are validated; true is acceptable if saved as-is).
- Unit: `tests/state/test_preset_manager_errors.cpp::test_unicode_preset_name_roundtrip` — `savePreset("テスト")`. Call `loadPreset("テスト")`. Require return true and loaded parameters match saved parameters.
- Unit: `tests/state/test_preset_manager_errors.cpp::test_concurrent_save_load_no_crash` — Spawn two std::threads: one repeatedly saves, the other repeatedly loads for 200 ms. Neither thread should crash, throw, or produce a data race (run with ThreadSanitizer if available; otherwise just require no crash).

## Technical Details
- For truncated XML: use `std::ofstream` in the test to write a partial preset file directly to the preset directory.
- For read-only path test: create a temp directory, `chmod 000` it on Linux, run the test, then `chmod 755` for cleanup (use RAII guard).
- ThreadSanitizer: add a CMake option note in the task, but the test must at minimum not crash under normal conditions.

## Dependencies
None
