# Task 219: Fix PresetManager::savePreset() Silent Failure

## Description
`PresetManager::savePreset()` has two bugs:

1. **Silent failure**: If `FileOutputStream::openedOk()` returns false (disk full, permission denied, etc.), the method proceeds silently and still updates `currentPresetName` to the unsaved name. The UI will show the preset as saved when no file was written.

2. **Inconsistent API**: `loadPreset()` returns `bool` to indicate success/failure. `savePreset()` returns `void`. Callers have no way to know if a save succeeded.

Fix both issues:
- Change `savePreset()` signature to return `bool`.
- Move the `currentPresetName = name` assignment inside the block where the file was successfully written.
- Return `true` on success, `false` on any failure (null XML, stream not opened).
- Update the call site in `PluginProcessor.cpp` (or wherever `savePreset()` is called) to handle the return value.

## Produces
None

## Consumes
None

## Relevant Files
Read: `M-LIM/src/state/PresetManager.h` — update savePreset() declaration
Modify: `M-LIM/src/state/PresetManager.h` — change `void savePreset(...)` to `bool savePreset(...)`
Modify: `M-LIM/src/state/PresetManager.cpp` — fix logic and return value
Read: `M-LIM/src/PluginProcessor.cpp` — find savePreset() call site
Modify: `M-LIM/src/PluginProcessor.cpp` — update to handle bool return (or discard it explicitly with `(void)`)

## Acceptance Criteria
- [ ] Run: `grep -n "bool savePreset" M-LIM/src/state/PresetManager.h` → Expected: declaration line found
- [ ] Run: `cmake --build build -j4 2>&1 | tail -5` → Expected: build succeeds, exit 0
- [ ] Run: `cd build && ctest -R preset --output-on-failure 2>&1 | tail -10` → Expected: all preset tests pass

## Tests
Unit: `tests/state/test_preset_manager_errors.cpp` — add a test that calls `savePreset()` with an unwritable path and verifies it returns `false` and `getCurrentPresetName()` is NOT updated to the failed name.

## Technical Details
New savePreset logic:
```cpp
bool PresetManager::savePreset(juce::AudioProcessorValueTreeState& apvts, const juce::String& name) {
    auto xml = apvts.createXml();
    if (xml == nullptr) return false;

    auto file = presetFileForName(name);
    juce::FileOutputStream stream(file);
    if (!stream.openedOk()) return false;

    stream.setPosition(0);
    stream.truncate();
    xml->writeTo(stream);
    currentPresetName = name;
    return true;
}
```

## Dependencies
None
