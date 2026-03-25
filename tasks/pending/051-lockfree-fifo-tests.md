# Task 051: LockFreeFIFO Unit Tests

## Description
The `LockFreeFIFO<MeterData>` is the sole communication channel between the audio thread and UI thread — a critical path with zero test coverage. Write unit tests verifying correctness of push/pop semantics, full-queue behavior, empty-queue behavior, and single-producer single-consumer ordering guarantees.

## Produces
None

## Consumes
MeterDataInterface

## Relevant Files
Create: `M-LIM/tests/dsp/test_lockfree_fifo.cpp` — LockFreeFIFO unit tests
Read: `M-LIM/src/PluginProcessor.h` — LockFreeFIFO and MeterData definitions

## Acceptance Criteria
- [ ] Run: `cd M-LIM && cmake --build build --target MLIMTests -j$(nproc) && cd build && ctest -R test_lockfree_fifo --output-on-failure` → Expected: all tests pass

## Tests
- Unit: `tests/dsp/test_lockfree_fifo.cpp::test_push_pop_single_item` — push one MeterData, pop it, verify all fields match
- Unit: `tests/dsp/test_lockfree_fifo.cpp::test_fifo_ordering` — push N items, pop N items, verify FIFO order preserved
- Unit: `tests/dsp/test_lockfree_fifo.cpp::test_pop_empty_returns_false` — pop from empty FIFO returns false/default without crash
- Unit: `tests/dsp/test_lockfree_fifo.cpp::test_push_full_drops_or_fails` — push beyond capacity should not crash, verify behavior (drop oldest or reject)
- Unit: `tests/dsp/test_lockfree_fifo.cpp::test_meter_data_default_values` — default-constructed MeterData has sane values (zeros, not NaN)
- Unit: `tests/dsp/test_lockfree_fifo.cpp::test_rapid_push_pop_cycle` — rapidly push and pop 10000 items, verify no corruption

## Technical Details
- Test both the FIFO container and the MeterData struct it carries
- Verify waveformBuffer array in MeterData copies correctly through FIFO
- Check that MeterData fields (inputLevelL/R, outputLevelL/R, gainReduction, truePeakL/R) survive push/pop roundtrip
- If FIFO has a fixed capacity, test boundary at exactly capacity items

## Dependencies
Requires task 013
