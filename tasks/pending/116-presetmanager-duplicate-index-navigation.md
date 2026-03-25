# Task 116: PresetManager Has Four Functions with Duplicate Index-Navigation Logic

## Description
`PresetManager.cpp` contains four separate functions that each independently
compute the next/previous preset index using identical logic:

- `loadNextPreset()` lines 70–86: calls `getPresetNames()`, advances index
- `loadPreviousPreset()` lines 88–102: calls `getPresetNames()`, retreats index
- `nextPreset()` lines 104–117: same advance logic, no load
- `previousPreset()` lines 119–131: same retreat logic, no load

The index-advance pattern is:
```cpp
auto names = getPresetNames();     // filesystem scan every call
if (names.isEmpty()) return;
int index = names.indexOf(currentPresetName);
if (index < 0) index = 0;
else index = (index + 1) % names.size();
currentPresetName = names[index];
```

This is duplicated four times with tiny mechanical variations (±1, optional load).
Each caller also triggers its own `getPresetNames()` filesystem scan.

Fix:
1. Add a private helper:
   ```cpp
   juce::String PresetManager::stepPreset(int direction);
   // direction: +1 = next, -1 = previous
   // Returns the new preset name after advancing, or empty string if no presets.
   ```
2. Implement it once, calling `getPresetNames()` a single time.
3. Rewrite all four public functions to delegate to `stepPreset()`.

`loadNextPreset` and `loadPreviousPreset` are the primary public API (task 083);
`nextPreset()` and `previousPreset()` may be removed if they have no callers
outside tests — check first.

## Produces
None

## Consumes
None

## Relevant Files
Modify: `M-LIM/src/state/PresetManager.h` — add private stepPreset() declaration (and remove bare next/prev if unused)
Modify: `M-LIM/src/state/PresetManager.cpp` — implement stepPreset(), simplify four public functions
Read: `M-LIM/src/PluginProcessor.cpp` — check if nextPreset()/previousPreset() are called anywhere
Read: `M-LIM/src/ui/TopBar.cpp` — check if bare advance methods are used

## Acceptance Criteria
- [ ] Run: `grep -c "getPresetNames()" M-LIM/src/state/PresetManager.cpp` → Expected: 1 (only inside stepPreset helper, not duplicated across multiple methods)
- [ ] Run: `cd M-LIM && cmake --build build -j$(nproc) 2>&1 | tail -5` → Expected: build succeeds
- [ ] Run: `cd M-LIM && cd build && ctest -R preset --output-on-failure 2>&1 | tail -10` → Expected: all preset tests pass

## Tests
- Unit: `tests/state/test_preset_manager.cpp` — ensure existing next/previous/load tests still pass; add a test that calls `loadNextPreset` twice and verifies the cursor advances correctly from the last-loaded name.

## Technical Details
- The `stepPreset()` helper is purely internal and should not appear in the
  public header API.
- If `nextPreset()` and `previousPreset()` have no external callers, remove them
  to reduce API surface. If they have callers, keep them as thin wrappers around
  `stepPreset()`.

## Dependencies
None
