# Task 075: Fix Undefined Behaviour — apvts Initialized Before undoManager

## Description
In `PluginProcessor.h`, the member declaration order is:

```cpp
// line 47
juce::AudioProcessorValueTreeState apvts;
// ...
// line 52
UndoManager undoManager;
```

C++ initialises non-static data members **in their declaration order**, regardless of the order
they appear in the constructor's member-initialiser list. The constructor initialiser is:

```cpp
apvts (*this, &undoManager.getJuceUndoManager(), "Parameters", createParameterLayout())
```

Because `apvts` is declared before `undoManager`, `undoManager` has not yet been constructed
when `apvts`'s constructor runs. Calling `undoManager.getJuceUndoManager()` at that point is
**undefined behaviour** — the `juce::UndoManager` member inside `UndoManager` does not exist
yet. This can manifest as random crashes or heap corruption, especially on Debug builds with
JUCE's leak detector.

## Produces
None

## Consumes
None

## Relevant Files
Modify: `src/PluginProcessor.h` — reorder member declarations so `undoManager` is declared before `apvts`

## Acceptance Criteria
- [ ] Run: `cmake --build build -j$(nproc) 2>&1 | grep -i "error"` → Expected: no compile errors
- [ ] Run: `cd build && ctest --output-on-failure 2>&1 | tail -10` → Expected: all tests pass

## Tests
None — this is a structural fix with no new test logic. Existing integration tests in
`tests/integration/test_plugin_processor.cpp` exercise the constructor path and will catch
regressions.

## Technical Details
In `src/PluginProcessor.h`, move the `undoManager` (and optionally `abState`,
`presetManager`) declarations to appear **before** `apvts` in the class body.
The correct order is:

```cpp
// Must be constructed before apvts
UndoManager   undoManager;
ABState       abState;
PresetManager presetManager;

// Depends on undoManager being alive
juce::AudioProcessorValueTreeState apvts;
```

No changes to `PluginProcessor.cpp` are needed — the initialiser list order doesn't matter;
only the declaration order in the header controls construction order.

## Dependencies
None
