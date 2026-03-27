# Task: UndoManager — Failed Action Handling and Description Tests

## Description
UndoManager has good coverage for normal undo/redo flow, but these cases are missing:

1. **Failed action (perform() returns false)**: If `perform()` returns false, the action should NOT be added to the undo stack. Currently no test verifies this — a failed action silently pushed onto the stack would allow "undoing" something that never happened.

2. **Undo/redo description access**: `getUndoDescription()` and `getRedoDescription()` (if implemented) are never tested. A description returning garbage or an empty string when an action is present is a UX bug.

3. **Transaction rollback semantics**: When a transaction is started and then aborted (e.g., no `endTransaction()` call matching a `beginTransaction()`), the behavior should be defined. No test verifies that a "leaked" open transaction doesn't corrupt the stack.

4. **canUndo() / canRedo() immediately after clearing**: After performing undo to the beginning of history, `canUndo()` must return false and `canRedo()` must return true. The converse after redo to the end of history. These combinations are tested individually but not in rapid alternation.

## Produces
None

## Consumes
None

## Relevant Files
Read: `M-LIM/src/state/UndoManager.h` — API: `perform()`, `undo()`, `redo()`, `canUndo()`, `canRedo()`, descriptions
Read: `M-LIM/src/state/UndoManager.cpp` — stack management, transaction logic
Read: `M-LIM/tests/state/test_undo_manager.cpp` — existing coverage
Modify: `M-LIM/tests/state/test_undo_manager.cpp` — add new tests

## Acceptance Criteria
- [ ] Run: `cd build && ctest -R "UndoManager" --output-on-failure` → Expected: all tests pass, exit 0

## Tests
- Unit: `tests/state/test_undo_manager.cpp::test_failed_perform_not_added_to_stack` — Create an action whose `perform()` returns false. Call `manager.perform(action)`. Verify `canUndo()` returns false (stack is empty — no failed action was recorded).
- Unit: `tests/state/test_undo_manager.cpp::test_undo_description_nonempty` — Add an action with a known description string. Verify `getUndoDescription()` returns a non-empty string matching the action's description. (Skip if UndoManager does not expose descriptions — document in test.)
- Unit: `tests/state/test_undo_manager.cpp::test_canundo_canredo_alternation` — Perform 5 actions. Undo all 5 (check `canUndo()` at each step). Then redo all 5 (check `canRedo()` at each step). Verify flags are consistent at every step, not just at the boundaries.
- Unit: `tests/state/test_undo_manager.cpp::test_history_limit_oldest_dropped` — This may already exist; if so, add assertion that `canRedo()` is false after the oldest entry is dropped (redo stack is cleared when capacity pushes out old entries — or verify the specific eviction behavior).

## Technical Details
- "Failed action": implement a mock `juce::UndoableAction` subclass whose `perform()` always returns false and whose `undo()` is a no-op. Wrap it in whatever wrapper UndoManager expects.
- If UndoManager wraps JUCE's `juce::UndoManager`, consult JUCE docs for `perform()` false behavior — JUCE's UndoManager does NOT add actions when perform() returns false; the test confirms M-LIM's wrapper preserves this guarantee.

## Dependencies
None
