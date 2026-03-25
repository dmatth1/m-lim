# Task 205: Remove UndoManager Thin Wrapper

## Description
`src/state/UndoManager.h/.cpp` is a class that wraps `juce::UndoManager` with 1:1 forwarding methods and adds no additional functionality, validation, or state. It exists purely as an unnecessary indirection layer:

```cpp
void UndoManager::undo()     { mJuceManager.undo(); }
void UndoManager::redo()     { mJuceManager.redo(); }
void UndoManager::beginNewTransaction() { mJuceManager.beginNewTransaction(); }
```

Remove this wrapper and replace all usages with `juce::UndoManager` directly. The `PluginProcessor` (which likely owns an `UndoManager`) should hold a `juce::UndoManager` member instead. The `TopBar` (which likely calls undo/redo via callback) passes through a callback; no change needed there unless it holds a pointer to `UndoManager`.

## Produces
None

## Consumes
None

## Relevant Files
Read: `M-LIM/src/state/UndoManager.h` — understand what methods are exposed
Read: `M-LIM/src/state/UndoManager.cpp` — confirm thin wrapper pattern
Read: `M-LIM/src/PluginProcessor.h` — find where UndoManager is owned
Read: `M-LIM/src/PluginProcessor.cpp` — find all call sites
Read: `M-LIM/src/ui/TopBar.h` — check if TopBar holds an UndoManager pointer
Read: `M-LIM/tests/state/test_undo_manager.cpp` — understand what tests exist
Modify: `M-LIM/src/PluginProcessor.h` — replace `UndoManager mUndoManager` with `juce::UndoManager mUndoManager`
Modify: `M-LIM/src/PluginProcessor.cpp` — update all call sites; remove `#include "state/UndoManager.h"`
Modify: `M-LIM/tests/state/test_undo_manager.cpp` — update tests to use `juce::UndoManager` directly

## Acceptance Criteria
- [ ] Run: `ls M-LIM/src/state/UndoManager.h M-LIM/src/state/UndoManager.cpp` → Expected: files do not exist (deleted)
- [ ] Run: `grep -rn "state/UndoManager" M-LIM/src/` → Expected: zero results
- [ ] Run: `cmake --build build -j4 2>&1 | tail -5` → Expected: build succeeds, exit 0
- [ ] Run: `cd build && ctest -R undo --output-on-failure 2>&1 | tail -5` → Expected: all undo tests pass

## Tests
None (existing undo tests should be updated in-place to use juce::UndoManager directly)

## Technical Details
- Also delete `UndoManager.h` and `UndoManager.cpp` from disk and remove them from `CMakeLists.txt`.
- If `CMakeLists.txt` uses a glob for sources, the deletion is sufficient. If sources are listed explicitly, remove the two file entries.
- `juce::UndoManager` requires no constructor arguments for default use; `UndoManager::UndoManager()` likely called the default constructor anyway.

## Dependencies
None
