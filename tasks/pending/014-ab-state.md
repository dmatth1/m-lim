# Task 014: A/B State Comparison

## Description
Implement A/B comparison state management that allows users to snapshot parameter states and toggle between two configurations for quick comparison.

## Produces
Implements: `ABStateInterface`

## Consumes
None

## Relevant Files
Create: `M-LIM/src/state/ABState.h` — class declaration
Create: `M-LIM/src/state/ABState.cpp` — implementation
Create: `M-LIM/tests/state/test_ab_state.cpp` — unit tests
Read: `SPEC.md` — ABStateInterface

## Acceptance Criteria
- [ ] Run: `cd M-LIM && cmake --build build --target MLIMTests -j$(nproc) && cd build && ctest -R test_ab_state --output-on-failure` → Expected: all tests pass

## Tests
- Unit: `tests/state/test_ab_state.cpp::test_capture_and_restore` — capture state A, change params, restore A, verify original values
- Unit: `tests/state/test_ab_state.cpp::test_toggle_switches` — toggle switches between A and B states
- Unit: `tests/state/test_ab_state.cpp::test_copy_a_to_b` — copy A to B makes both identical

## Technical Details
- Store snapshots as `juce::ValueTree` copies from APVTS state
- captureState: deep copy current APVTS ValueTree
- restoreState: replace APVTS ValueTree with stored copy
- toggle: swap active state (A↔B), capture current before switching, restore the other
- Default: state A is active, both start as copies of initial state

## Dependencies
Requires task 001
