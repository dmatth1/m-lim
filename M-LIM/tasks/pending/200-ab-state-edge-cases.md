# Task 200: ABState — missing edge-case and multi-parameter tests

## Description
`test_ab_state.cpp` only uses a minimal 2-parameter APVTS (gain + ceiling) and does not cover
toggle symmetry, copy operations, or scenarios with more parameters.

Add tests for:

1. **Toggle is symmetric** — capture A, change params, toggle (saves current as B, restores A);
   the params must match what was captured as A. Toggle again (saves current as A, restores B);
   params must match what was set before the first toggle.
2. **`copyAtoB()` makes B identical to A** — capture state A, change params, capture state B
   (different values), then call `copyAtoB()`; toggle to B; params must match A's values.
3. **`copyBtoA()` makes A identical to B** — symmetrical test for `copyBtoA()`.
4. **Restore without prior capture returns valid state** — call `restoreState()` on a freshly
   constructed `ABState` (nothing captured); should not crash and APVTS values must be unchanged
   (or default, depending on implementation).
5. **Multiple parameters round-trip** — extend the test processor to include 5 parameters
   (gain, ceiling, attack, release, lookahead), capture/restore/toggle all five and verify all
   five are correctly preserved.
6. **Active slot tracks correctly across toggle** — call `toggle()` twice from the same state;
   `isA()` must return `true` after 0 toggles, `false` after 1, `true` after 2.

## Produces
None

## Consumes
None

## Relevant Files
Modify: `tests/state/test_ab_state.cpp` — add new TEST_CASEs and an extended processor helper
Read: `src/state/ABState.h` — captureState(), restoreState(), toggle(), copyAtoB(), copyBtoA(), isA()

## Acceptance Criteria
- [ ] Run: `cd /workspace/M-LIM/build && ctest -R "ABState" --output-on-failure` → Expected: all ABState tests pass, exit 0

## Tests
- Unit: `tests/state/test_ab_state.cpp::test_toggle_symmetry` — double toggle restores original state
- Unit: `tests/state/test_ab_state.cpp::test_copy_a_to_b` — copyAtoB() makes B identical to A
- Unit: `tests/state/test_ab_state.cpp::test_copy_b_to_a` — copyBtoA() makes A identical to B
- Unit: `tests/state/test_ab_state.cpp::test_restore_without_capture` — restoreState() on fresh ABState does not crash
- Unit: `tests/state/test_ab_state.cpp::test_five_parameter_round_trip` — 5-parameter APVTS, all params preserved through capture/restore/toggle
- Unit: `tests/state/test_ab_state.cpp::test_is_a_tracks_toggle` — isA() alternates correctly with each toggle() call

## Technical Details
- For `test_restore_without_capture`: A fresh ABState holds empty `juce::ValueTree` for both slots.
  Calling `restoreState()` on it will attempt to copy an empty tree into the APVTS. The test should
  verify no exception/crash occurs; actual parameter values are implementation-defined.
- For the 5-parameter test: create a new inner class `ABTestProcessor5` (or a templated helper)
  with parameters: "gain"(-60..12,0), "ceiling"(-30..0,-0.1), "attack"(0..100,10),
  "release"(10..1000,100), "lookahead"(0..5,2).
- Reuse the existing `getParam()` / `setParam()` helpers from the file.
- For `test_toggle_symmetry`: set specific values, call `captureState(apvts)` (captures A),
  change all params to new values, call `toggle(apvts)` (captures new state as B, restores A),
  verify all params match original values; then toggle again and verify B values are restored.

## Dependencies
None
