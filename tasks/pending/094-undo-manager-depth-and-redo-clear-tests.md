# Task 094: UndoManager Depth, Redo-Clear, and Multi-Step Tests

## Description
`test_undo_manager.cpp` has only 3 test cases covering single-step undo/redo and the canUndo/canRedo flags. Missing coverage:

- Multi-step undo (3+ actions, undo each step, verify correct intermediate values)
- Redo stack is cleared when a new action is performed after an undo
- Multiple redos after multiple undos work correctly
- `beginNewTransaction()` groups actions (multiple performs in one transaction = single undo step)
- Stack behavior with many actions (100+) does not crash or corrupt state

## Produces
None

## Consumes
None

## Relevant Files
Read: `src/state/UndoManager.h` — wrapper around juce::UndoManager
Read: `src/state/UndoManager.cpp` — implementation
Modify: `tests/state/test_undo_manager.cpp` — add new test cases

## Acceptance Criteria
- [ ] Run: `cd /workspace/M-LIM/build && ctest -R UndoManager --output-on-failure` → Expected: all tests pass, exit 0
- [ ] Run: `grep -c "TEST_CASE" /workspace/M-LIM/tests/state/test_undo_manager.cpp` → Expected: at least 6 test cases

## Tests
- Unit: `tests/state/test_undo_manager.cpp::test_multi_step_undo` — perform 5 separate transactions, undo each one, verify value returns to initial state
- Unit: `tests/state/test_undo_manager.cpp::test_new_action_clears_redo_stack` — undo once, perform a new action, canRedo() must be false
- Unit: `tests/state/test_undo_manager.cpp::test_transaction_grouping` — two performs inside one transaction = one undo step (both values revert together)
- Unit: `tests/state/test_undo_manager.cpp::test_many_actions_no_crash` — 100 transactions created then all undone one by one without crash

## Technical Details
- Re-use the `SetValueAction` helper already defined in the test file
- For transaction grouping: call `beginNewTransaction()`, `perform(A)`, `perform(B)`, then `undo()` should revert both A and B
- Note: juce::UndoManager groups actions within the same transaction

## Dependencies
None
