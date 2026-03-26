# Task 245: LockFreeFIFO Non-Power-of-Two Capacity and Wraparound Tests

## Description
`tests/dsp/test_lockfree_fifo.cpp` covers push/pop, ordering, overflow, and concurrent producer/consumer, but is missing:

1. **Non-power-of-two construction**: `LockFreeFIFO<MeterData>` rounds capacity up to the next power of two. No test verifies this behavior explicitly — e.g., constructing with capacity=3 should result in `capacity()` returning 4 (or 4 − 1 = 3 usable slots). Tests use 4, 8, 64, 256 (all powers of two) which may hide a rounding bug.

2. **Fill, drain, fill again (wraparound)**: No test fills the FIFO to capacity, drains it completely, then fills it again. This exercises the ring buffer's head/tail pointer wraparound at the power-of-2 boundary. The existing `test_rapid_push_pop_cycle` does many iterations but it's a one-in-one-out pattern, not a true fill/drain/fill cycle.

3. **Capacity = 1 construction**: Edge case — a FIFO with capacity=1 should round up to 2 (power of two), and be able to hold 1 item (since ring buffers use size−1 as max). No test covers this degenerate case.

## Produces
None

## Consumes
None

## Relevant Files
Read: `tests/dsp/test_lockfree_fifo.cpp` — existing tests to extend
Read: `src/dsp/MeterData.h` — LockFreeFIFO template definition
Modify: `tests/dsp/test_lockfree_fifo.cpp` — add the new test cases

## Acceptance Criteria
- [ ] Run: `cd /workspace/M-LIM/build && ./tests/MLIMTests "[lockfree_fifo]" --reporter compact` → Expected: all tests pass

## Tests
- Unit: `tests/dsp/test_lockfree_fifo.cpp::test_non_power_of_two_capacity_rounds_up` — construct with capacity=3, 5, 6, 7 and verify `fifo.capacity()` is a power of 2 greater than the requested size
- Unit: `tests/dsp/test_lockfree_fifo.cpp::test_fill_drain_fill_wraparound` — construct with capacity=4, push 3 items, pop all 3, push 3 more items (crossing the ring boundary), pop all 3 and verify values match (no data loss or corruption at wraparound)
- Unit: `tests/dsp/test_lockfree_fifo.cpp::test_capacity_one_construction` — construct with capacity=1, verify `capacity()` >= 1 and at least 1 item can be pushed and popped without crash

## Technical Details
- Power-of-two check: `(cap & (cap - 1)) == 0` for any cap > 0
- The wraparound test must push distinct sentinel values (e.g., `inputLevelL = 10.0f`, `11.0f`, `12.0f`) so corruption is detectable by value mismatch
- Read `MeterData.h` to confirm the template parameter for `LockFreeFIFO` — the test already instantiates `LockFreeFIFO<MeterData>`

## Dependencies
None
