# Task 446: PresetManager — Concurrent Access and File Corruption Recovery Tests

## Description
Missing PresetManager robustness tests:
1. Truncated XML (partial write) fails gracefully — return false, not crash
2. Failed save (unwritable dir) does not add preset to index
3. Whitespace-only preset name does not crash
4. Unicode preset name roundtrip
5. Concurrent save/load does not crash

## Produces
None

## Consumes
None

## Relevant Files
Read: `src/state/PresetManager.h` — save/load/navigate API
Read: `src/state/PresetManager.cpp` — file I/O implementation
Read: `tests/state/test_preset_manager_errors.cpp` — existing error coverage
Modify: `tests/state/test_preset_manager_errors.cpp` — add edge cases

## Acceptance Criteria
- [ ] Run: `cd build && ctest -R "PresetManager" --output-on-failure` → Expected: all tests pass, exit 0

## Tests
- Unit: `tests/state/test_preset_manager_errors.cpp::test_truncated_xml_load_returns_false` — write first 20 bytes of valid preset file; loadPreset() returns false, no crash
- Unit: `tests/state/test_preset_manager_errors.cpp::test_failed_save_does_not_add_to_index` — read-only preset dir; getPresetNames() does not include unsaved preset
- Unit: `tests/state/test_preset_manager_errors.cpp::test_whitespace_only_name_no_crash` — savePreset("   "); must not crash
- Unit: `tests/state/test_preset_manager_errors.cpp::test_unicode_preset_name_roundtrip` — save/load "テスト"; parameters match
- Unit: `tests/state/test_preset_manager_errors.cpp::test_concurrent_save_load_no_crash` — two threads: one saves, one loads for 200 ms; no crash

## Technical Details
For read-only path: `chmod 000` the temp dir, run test, `chmod 755` in RAII cleanup (Linux).
Truncated XML: write 20 bytes of a valid preset via `std::ofstream` directly.

## Dependencies
None
