# Task 167: Add Tests for ABState::copyBtoA()

## Description
`ABState::copyBtoA()` (ABState.cpp:42-48) is a public method that copies slot B's snapshot into
slot A's APVTS state, but it has **no test coverage whatsoever**. Only `copyAtoB()` is exercised
in the test suite (test_ab_state.cpp:131).  Since A/B copy in both directions is a user-facing
feature, the missing direction must be covered.

Tests to add in `test_ab_state.cpp`:

1. **test_copy_b_to_a_restores_values** — Capture state A with one parameter value, toggle to B,
   set a different parameter value, capture B, then call `copyBtoA()`, restore state.  Verify that
   A's parameter value now matches what was captured in B.

2. **test_copy_b_to_a_when_b_not_captured_no_crash** — Call `copyBtoA()` without ever calling
   `captureB()`.  Assert no exception is thrown and the APVTS state is not corrupted (parameters
   still have finite values).

3. **test_copy_a_to_b_and_b_to_a_roundtrip** — Capture A, toggle to B, mutate parameters, capture
   B, call `copyAtoB()`, verify A has B's values; then call `copyBtoA()` and verify B's state is
   again readable (symmetric round-trip).

## Produces
None

## Consumes
None

## Relevant Files
Read: `M-LIM/src/state/ABState.h:30-36` — copyAtoB and copyBtoA declarations
Read: `M-LIM/src/state/ABState.cpp:36-48` — both copy implementations
Modify: `M-LIM/tests/state/test_ab_state.cpp` — add three new TEST_CASE blocks

## Acceptance Criteria
- [ ] Run: `cd M-LIM/build && ctest -R test_ab_state --output-on-failure` → Expected: all tests pass including the three new copyBtoA tests
- [ ] Run: `grep -c "copyBtoA" M-LIM/tests/state/test_ab_state.cpp` → Expected: output >= 3

## Tests
- Unit: `tests/state/test_ab_state.cpp::test_copy_b_to_a_restores_values` — values in A match B after copyBtoA
- Unit: `tests/state/test_ab_state.cpp::test_copy_b_to_a_when_b_not_captured_no_crash` — defensive: no capture before copy
- Unit: `tests/state/test_ab_state.cpp::test_copy_a_to_b_and_b_to_a_roundtrip` — symmetric round-trip consistency

## Technical Details
- `copyBtoA()` is at ABState.cpp:42; it calls `apvts.replaceState(stateB.createCopy())` (or
  equivalent).  Inspect the actual implementation before writing the test to match the correct
  semantics.
- Use the same `MockAPVTS` / test APVTS fixture used elsewhere in `test_ab_state.cpp`.
- A "not captured" B slot likely holds a default-constructed ValueTree; the test should verify
  `REQUIRE_NOTHROW` and that parameter values are still finite after the call.

## Dependencies
None
