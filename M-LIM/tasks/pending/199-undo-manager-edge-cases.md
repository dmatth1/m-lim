# Task 199: UndoManager — missing edge-case tests

## Description
`test_undo_manager.cpp` covers the basic happy path (undo/redo a single value change) and the
`canUndo()`/`canRedo()` flags. It is missing:

1. **Undo with empty history** — `manager.undo()` on a freshly created `UndoManager` must return
   `false` (not crash). Same for `redo()`.
2. **Undo past the beginning of history** — perform one transaction, undo it, then undo again;
   the second undo must return `false` and leave state unchanged.
3. **Redo past the end of history** — perform one transaction, undo, redo, then redo again; the
   second redo must return `false`.
4. **Multiple transactions, multi-step undo/redo** — record three separate transactions (value→10,
   value→20, value→30), then undo three times in succession verifying intermediate values (20, 10,
   0), then redo twice verifying (10, 20).
5. **New transaction after partial undo truncates redo history** — undo twice, then begin a new
   transaction and perform a new action; verify `canRedo()` is now false (new action clears redo).
6. **`beginNewTransaction()` grouping** — two actions inside the same transaction block are undone
   atomically: perform action A then action B without calling `beginNewTransaction()` between them,
   then call `undo()` once; both A and B should be reverted.

## Produces
None

## Consumes
None

## Relevant Files
Modify: `tests/state/test_undo_manager.cpp` — add new TEST_CASEs
Read: `src/state/UndoManager.h` — public API: beginNewTransaction(), undo(), redo(), canUndo(), canRedo()

## Acceptance Criteria
- [ ] Run: `cd /workspace/M-LIM/build && ctest -R "UndoManager" --output-on-failure` → Expected: all undo manager tests pass, exit 0

## Tests
- Unit: `tests/state/test_undo_manager.cpp::test_undo_empty_history` — undo() returns false, no crash
- Unit: `tests/state/test_undo_manager.cpp::test_undo_past_beginning` — second undo returns false; value unchanged
- Unit: `tests/state/test_undo_manager.cpp::test_redo_past_end` — second redo returns false
- Unit: `tests/state/test_undo_manager.cpp::test_multi_step_undo_redo` — three transactions, three undos, two redos with correct intermediate values
- Unit: `tests/state/test_undo_manager.cpp::test_new_transaction_clears_redo` — undo then new action clears redo history
- Unit: `tests/state/test_undo_manager.cpp::test_transaction_grouping` — two actions in one transaction undone atomically

## Technical Details
- Reuse the existing `SetValueAction` helper class already defined in `test_undo_manager.cpp`.
- For test_transaction_grouping: call `manager.beginNewTransaction()` once, then `juce.perform()`
  twice with different target variables; call `manager.undo()` once; both targets must revert.
- `juce::UndoManager` groups actions within the same transaction until `beginNewTransaction()` is
  called. The grouping test verifies this contract is preserved by the wrapper.

## Dependencies
None
