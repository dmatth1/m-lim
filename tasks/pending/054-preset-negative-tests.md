# Task 054: Preset Manager Negative and Error Case Tests

## Description
Task 015 only tests the happy path (save, load, navigate). A preset system must handle malformed XML, missing files, empty names, duplicate names, and corrupt data without crashing. These are common real-world failures when users manually edit preset files or when files get corrupted.

## Produces
None

## Consumes
PresetManagerInterface

## Relevant Files
Create: `M-LIM/tests/state/test_preset_manager_errors.cpp` — negative test cases
Read: `M-LIM/src/state/PresetManager.h` — PresetManager interface

## Acceptance Criteria
- [ ] Run: `cd M-LIM && cmake --build build --target MLIMTests -j$(nproc) && cd build && ctest -R test_preset_manager_errors --output-on-failure` → Expected: all tests pass

## Tests
- Unit: `tests/state/test_preset_manager_errors.cpp::test_load_nonexistent_preset` — loadPreset("doesnotexist") returns false without crash
- Unit: `tests/state/test_preset_manager_errors.cpp::test_load_malformed_xml` — write a malformed XML file to preset dir, attempt load, verify returns false and does not corrupt current state
- Unit: `tests/state/test_preset_manager_errors.cpp::test_save_empty_name` — savePreset("", ...) should handle gracefully (reject or use default name)
- Unit: `tests/state/test_preset_manager_errors.cpp::test_next_previous_wraps_or_clamps` — calling nextPreset() past the last preset should wrap or clamp, not crash
- Unit: `tests/state/test_preset_manager_errors.cpp::test_previous_at_start` — calling previousPreset() at first preset should wrap or clamp, not crash
- Unit: `tests/state/test_preset_manager_errors.cpp::test_load_preset_missing_parameters` — XML preset with some parameters missing should load without crash, missing params get defaults

## Technical Details
- Create a temporary directory for test presets to avoid polluting real preset storage
- For malformed XML test, write literal `<broken><xml` to a .xml file in preset directory
- For missing parameters test, write a valid XML that only contains 2-3 of the 17 parameters
- Verify that after any failed load, the current parameter state is unchanged

## Dependencies
Requires task 015
