# Task 527: Document ABState Thread Safety and Fix Uninitialized State

## Description
Two issues in ABState:

1. **No thread safety documentation**: ABState is message-thread-only but has no comments indicating this. A developer could mistakenly call it from the audio thread, causing ValueTree corruption.

2. **Uninitialized default state**: The default constructor (line 3-6 of `ABState.cpp`) leaves `stateA` and `stateB` as invalid/empty ValueTrees. `fromXml()` silently creates empty trees on parse failure. First access before `captureState()` uses invalid trees.

## Produces
None

## Consumes
None

## Relevant Files
Modify: `M-LIM/src/state/ABState.h` — add thread safety documentation, initialize member trees
Modify: `M-LIM/src/state/ABState.cpp` — add jasserts for invalid state access in restoreState

## Acceptance Criteria
- [ ] Run: `cd build && ctest --output-on-failure -R ab_state` → Expected: all ABState tests pass
- [ ] Run: `grep -n "Thread" M-LIM/src/state/ABState.h` → Expected: thread safety documentation present

## Tests
- Unit: `tests/state/test_ab_state.cpp::test_default_state_is_valid` — verify that default-constructed ABState has valid (if empty) trees

## Technical Details
- Add `/// @note Message thread only — not safe to call from the audio thread.` to class declaration
- Initialize stateA/stateB to `juce::ValueTree("ABState")` in default constructor
- Add `jassert(stateA.isValid())` / `jassert(stateB.isValid())` in restoreState()

## Dependencies
None
