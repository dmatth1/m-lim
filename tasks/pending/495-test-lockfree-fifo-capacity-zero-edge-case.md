# Task 495: LockFreeFIFO — Capacity-Zero and Concurrent Integrity Tests

## Description
The existing `LockFreeFIFO` tests cover: push/pop ordering, concurrent producer/consumer,
capacity-1 construction, and fill/drain/fill wraparound. Two gaps remain:

1. **Capacity zero construction**: `LockFreeFIFO<MeterData>(0)` — the non-power-of-two
   test shows 7→8 rounding up, but zero is not tested. What happens: push returns false
   (queue is always full), pop returns false (queue is always empty). If the capacity
   rounding rounds 0→0 and the mask becomes 0xFFFF or UB, this is a bug.

2. **Concurrent overwrite integrity**: The concurrent test verifies ordering but not
   that individual `MeterData` items are never partially written (torn reads). The
   test pushes and pops simple sequential counters; it does not verify that no item
   has mixed fields from two different writes (e.g., inputL from item N and outputL
   from item N+1). For a `MeterData` struct with multiple floats, torn reads could
   produce inconsistent meter state even with a lock-free FIFO.

## Produces
None

## Consumes
None

## Relevant Files
Read: `tests/dsp/test_lockfree_fifo.cpp` — existing tests
Read: `src/dsp/MeterData.h` — MeterData struct fields
Modify: `tests/dsp/test_lockfree_fifo.cpp` — add new test cases

## Acceptance Criteria
- [ ] Run: `cd build && ctest -R "lockfree_fifo" --output-on-failure` → Expected: all tests pass, exit 0

## Tests
- Unit: `tests/dsp/test_lockfree_fifo.cpp::test_capacity_zero_push_returns_false` — construct LockFreeFIFO<MeterData>(0); push must return false, isEmpty() must return true
- Unit: `tests/dsp/test_lockfree_fifo.cpp::test_capacity_zero_pop_returns_false` — same construction; pop must return false without crashing
- Unit: `tests/dsp/test_lockfree_fifo.cpp::test_concurrent_data_no_torn_reads` — producer pushes MeterData items where all fields = same float value (item i has all fields set to float(i)); consumer verifies that every popped item has all fields equal (no torn read across fields); run for 10000 iterations

## Technical Details
For the torn-read test, use a MeterData sentinel pattern:
```cpp
MeterData item;
item.inputL = item.inputR = item.outputL = item.outputR =
    item.gainReductionDb = static_cast<float>(i);
// Consumer side: verify all fields equal the first field
MeterData popped;
if (fifo.pop(popped)) {
    float ref = popped.inputL;
    REQUIRE(popped.inputR == ref);
    REQUIRE(popped.outputL == ref);
    // ...
}
```
If MeterData has more fields, check all of them. This detects any scenario where
the FIFO implementation allows a consumer to see a partially updated item.

## Dependencies
None
