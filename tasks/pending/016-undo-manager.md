# Task 016: Undo Manager

## Description
Implement undo/redo functionality wrapping JUCE's UndoManager, integrated with the APVTS parameter system.

## Produces
Implements: `UndoManagerInterface`

## Consumes
None

## Relevant Files
Create: `M-LIM/src/state/UndoManager.h` — class declaration (thin wrapper)
Create: `M-LIM/src/state/UndoManager.cpp` — implementation
Create: `M-LIM/tests/state/test_undo_manager.cpp` — unit tests
Read: `SPEC.md` — UndoManagerInterface

## Acceptance Criteria
- [ ] Run: `cd M-LIM && cmake --build build --target MLIMTests -j$(nproc) && cd build && ctest -R test_undo_manager --output-on-failure` → Expected: all tests pass

## Tests
- Unit: `tests/state/test_undo_manager.cpp::test_undo_reverts_change` — change param, undo, verify original value
- Unit: `tests/state/test_undo_manager.cpp::test_redo_restores_change` — undo then redo restores changed value
- Unit: `tests/state/test_undo_manager.cpp::test_can_undo_redo_flags` — canUndo/canRedo return correct state

## Technical Details
- Wraps `juce::UndoManager` instance
- APVTS constructor takes UndoManager pointer — automatic transaction recording
- beginNewTransaction creates a new undo point
- undo()/redo() delegate to juce::UndoManager
- canUndo()/canRedo() check if operations available
- UndoManager instance owned by PluginProcessor, passed to APVTS

## Dependencies
Requires task 001
