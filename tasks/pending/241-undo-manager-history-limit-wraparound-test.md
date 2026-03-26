# Task 241: UndoManager History Limit Wraparound Test

## Description
`tests/state/test_undo_manager.cpp` has `test_many_actions_no_crash` which performs 100 transactions and undoes them all, but the JUCE `UndoManager` has a configurable `maxNumActions` limit (default is no hard cap but there is a `maxNumActionsToKeep` parameter). More importantly, there is no test verifying the behavior when the undo history exceeds the limit — specifically:

1. **History limit boundary**: What happens after more transactions than the buffer can hold? Old entries must be silently dropped (not a crash), and `canUndo()` should return true for the remaining in-history transactions.
2. **undo() at exhaustion returns false**: After undoing everything available, `undo()` must return false, not crash.
3. **Multiple rapid undo/redo cycles**: Repeatedly undo-all then redo-all for many cycles without memory leak or state corruption.

Add tests that construct a `juce::UndoManager` with a small `maxNumActionsToKeep` and verify the above behaviors.

## Produces
None

## Consumes
None

## Relevant Files
Read: `tests/state/test_undo_manager.cpp` — existing tests to extend
Modify: `tests/state/test_undo_manager.cpp` — add the new test cases

## Acceptance Criteria
- [ ] Run: `cd /workspace/M-LIM/build && ./tests/MLIMTests "[UndoManager]" --reporter compact` → Expected: all tests pass including new ones

## Tests
- Unit: `tests/state/test_undo_manager.cpp::test_history_limit_drops_oldest_no_crash` — create UndoManager with small limit (e.g., 5), perform 20 transactions, verify canUndo() is true but only the most recent N are still undoable; verify no crash when undoing beyond retained history
- Unit: `tests/state/test_undo_manager.cpp::test_undo_returns_false_when_exhausted` — undo all available transactions, then call undo() once more and verify it returns false (not crash)
- Unit: `tests/state/test_undo_manager.cpp::test_rapid_undo_redo_cycles_stable` — perform 10 transactions, then undo-all/redo-all 50 times in a loop; verify final value is correct and `canRedo()` is false after all redos

## Technical Details
- `juce::UndoManager` constructor accepts `maxNumActionsToKeep` (default 30) and `minimumTransactionMilliseconds` (default 500ms, set to 0 for tests)
- To construct with explicit limits: `juce::UndoManager manager(5, 0)` — keeps only 5 actions, no time grouping
- Use the existing `SetValueAction` helper already defined in the test file
- For the limit test: after performing 20 transactions with limit=5, at most 5 should be undoable (exact behavior depends on JUCE version — test should be lenient: verify canUndo()==true and at most 5 undos succeed before canUndo()==false)

## Dependencies
None
