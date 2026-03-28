# Task: PresetManager — Path Traversal Prevention Tests

## Description
`PresetManager::presetFileForName()` calls `presetDirectory.getChildFile(name + ".xml")`
directly without sanitizing the name. `juce::File::getChildFile("../evil")` resolves to
a path *outside* the preset directory. A name like `"../AppData/config"` would create or
overwrite a file at `presetDirectory/../AppData/config.xml`.

The existing `test_special_chars_no_crash` tests `/` and `\` in names but does NOT test
`..`-based traversal, which is the actual attack vector.

Two separate issues to address:

1. **Test**: Verify that `savePreset("../evil", ...)` cannot write outside the preset dir
   (should either sanitize the name or return false/no-op). The test should check that no
   file was created outside `presetDirectory`.

2. **Test**: Verify that `loadPreset("../sensitive", ...)` cannot read outside the preset
   dir (should return false if the resolved path escapes the preset directory).

If the tests reveal that traversal IS possible, create a follow-up fix task. If the tests
reveal that JUCE's `getChildFile()` already prevents traversal (by stripping leading `../`),
document this in a comment. Either way, the test must exist to detect regressions.

## Produces
None

## Consumes
None

## Relevant Files
Read: `src/state/PresetManager.cpp` — presetFileForName() at line 108, savePreset() at line 16
Read: `src/state/PresetManager.h` — public API
Read: `tests/state/test_preset_manager.cpp` — existing test_special_chars_no_crash
Modify: `tests/state/test_preset_manager.cpp` — add path traversal tests

## Acceptance Criteria
- [ ] Run: `cd build && ctest -R "PresetManager" --output-on-failure` → Expected: all tests pass, exit 0

## Tests
- Unit: `tests/state/test_preset_manager.cpp::test_dotdot_traversal_save_no_escape` — savePreset("../evil", apvts); verify that no file named "evil.xml" exists outside `tmp.dir` (i.e., the parent directory must not contain "evil.xml")
- Unit: `tests/state/test_preset_manager.cpp::test_dotdot_traversal_load_returns_false` — loadPreset("../evil", apvts); must return false (no file read from outside preset dir)
- Unit: `tests/state/test_preset_manager.cpp::test_nested_dotdot_traversal_no_escape` — savePreset("../../deep/escape", apvts); same assertion as above

## Technical Details
After calling savePreset("../evil", apvts):
```cpp
auto escaped = tmp.dir.getParentDirectory().getChildFile("evil.xml");
REQUIRE_FALSE(escaped.existsAsFile()); // must not have been written
```
The test must clean up any escaped file if it accidentally gets created (use
`escaped.deleteFile()` in teardown). Note: JUCE on Linux may or may not resolve
`getChildFile("../")` to the parent — verify empirically. If traversal works,
create a separate fix task for sanitization in `presetFileForName()`.

## Dependencies
None
