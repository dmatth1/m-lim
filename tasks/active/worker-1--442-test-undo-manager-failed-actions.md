# Task 442: UndoManager — Failed Action Handling and Stack Consistency Tests

## Description
Missing UndoManager tests:
1. `perform()` returning false must NOT add to the undo stack
2. Undo/redo description strings are non-empty
3. `canUndo()`/`canRedo()` flags consistent in rapid alternation
4. History limit drops oldest entry correctly

## Produces
None

## Consumes
None

## Relevant Files
Read: `src/state/UndoManager.h` — perform(), undo(), redo(), canUndo(), canRedo(), descriptions
Read: `src/state/UndoManager.cpp` — stack management, transaction logic
Read: `tests/state/test_undo_manager.cpp` — existing coverage
Modify: `tests/state/test_undo_manager.cpp` — add new tests

## Acceptance Criteria
- [ ] Run: `cd build && ctest -R "UndoManager" --output-on-failure` → Expected: all tests pass, exit 0

## Tests
- Unit: `tests/state/test_undo_manager.cpp::test_failed_perform_not_added_to_stack` — action whose perform() returns false; canUndo() must be false afterward
- Unit: `tests/state/test_undo_manager.cpp::test_undo_description_nonempty` — action with known description; getUndoDescription() returns non-empty matching string
- Unit: `tests/state/test_undo_manager.cpp::test_canundo_canredo_alternation` — 5 actions, undo all, redo all; canUndo/canRedo flags correct at every step
- Unit: `tests/state/test_undo_manager.cpp::test_history_limit_oldest_dropped` — verify canRedo() = false after oldest-entry eviction

## Technical Details
Create a mock `juce::UndoableAction` subclass with `perform()` returning false for the
failed-action test. If UndoManager wraps JUCE's UndoManager, JUCE guarantees failed
actions are not added — test confirms M-LIM wrapper preserves this.

## Dependencies
None
