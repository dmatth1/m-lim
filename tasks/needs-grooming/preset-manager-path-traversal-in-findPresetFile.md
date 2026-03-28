# Task: Fix path traversal in PresetManager::findPresetFile()

## Description
`PresetManager::findPresetFile()` (line 122-136 of `src/state/PresetManager.cpp`) performs a recursive search via `findChildFiles()` but does not validate that the returned file is actually inside the preset directory. While `presetFileForName()` has an `isAChildOf()` guard, the recursive fallback path bypasses it entirely — a crafted preset name could resolve to files outside the preset directory.

Add `isAChildOf()` validation to the result of the recursive search before returning it.

## Produces
None

## Consumes
None

## Relevant Files
Modify: `M-LIM/src/state/PresetManager.cpp` — add isAChildOf guard at line 132-133
Read: `M-LIM/src/state/PresetManager.h` — class interface

## Acceptance Criteria
- [ ] Run: `grep -n "isAChildOf" M-LIM/src/state/PresetManager.cpp` → Expected: isAChildOf check appears in both presetFileForName AND findPresetFile
- [ ] Run: `cd build && ctest --output-on-failure -R preset` → Expected: all preset tests pass

## Tests
- Unit: `tests/state/test_preset_manager.cpp::test_findPresetFile_rejects_traversal` — verify that a name like `../../etc/passwd` returns an empty File from findPresetFile

## Technical Details
In `findPresetFile()`, after line 132, validate the found file:
```cpp
for (const auto& file : files)
    if (file.isAChildOf(presetDirectory))
        return file;
return {};
```
Also add preset name validation in `savePreset()` — reject empty names and names containing path separators (`/`, `\`).

## Dependencies
None
