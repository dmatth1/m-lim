# Task 080: ABState Isolation, Default B Behavior, and Toggle Correctness Tests

## Description
`test_ab_state.cpp` has only 3 test cases (capture A, toggle, copy A→B). Missing coverage:

- After construction, state B is empty/default. Toggling from A to B before capturing B should not overwrite current params with garbage.
- Changing parameters while on state A, then toggling to B and back to A, must restore exact A values.
- State isolation: modifying params while on B does not affect the stored A snapshot.
- `captureState()` called twice in a row (re-capturing A) overwrites the previous A snapshot.
- Toggle from B back to A restores the correct A values when B was modified after toggling.

## Produces
None

## Consumes
None

## Relevant Files
Read: `src/state/ABState.h` — captureState(), toggleAB(), copyAtoB() interface
Read: `src/state/ABState.cpp` — implementation
Modify: `tests/state/test_ab_state.cpp` — add test cases

## Acceptance Criteria
- [ ] Run: `cd /workspace/M-LIM/build && ctest -R ABState --output-on-failure` → Expected: all tests pass, exit 0
- [ ] Run: `grep -c "TEST_CASE" /workspace/M-LIM/tests/state/test_ab_state.cpp` → Expected: at least 6 test cases

## Tests
- Unit: `tests/state/test_ab_state.cpp::test_toggle_to_empty_b_no_crash` — call `toggleAB()` before capturing B → no crash, params remain valid
- Unit: `tests/state/test_ab_state.cpp::test_a_state_isolated_from_b_modifications` — capture A, toggle to B, change a param, toggle back to A → original A param value restored
- Unit: `tests/state/test_ab_state.cpp::test_recapture_a_overwrites_snapshot` — capture A with value X, change param to Y, capture A again, toggle away and back → value is Y not X
- Unit: `tests/state/test_ab_state.cpp::test_full_ab_cycle` — set params to known values, capture A, toggle to B, modify params, toggle back to A → all A values restored exactly

## Technical Details
- Use `apvts.getRawParameterValue(ParamID::inputGain)` to read/write a float param as the test anchor value
- Check that the ABState test file already imports Parameters.h for ParamID constants

## Dependencies
None
