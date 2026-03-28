# Task 528: Standardize Atomic Memory Ordering Across LimiterEngine

## Description
LimiterEngine uses inconsistent memory ordering for atomic operations. Some use `memory_order_relaxed` (correct for independent parameters), others use the default `memory_order_seq_cst` (unnecessarily strong). Since these atomics are independent parameter values where brief staleness is acceptable, all should use relaxed ordering consistently.

## Produces
None

## Consumes
None

## Relevant Files
Modify: `M-LIM/src/dsp/LimiterEngine.cpp` — standardize all atomic load/store to use relaxed ordering
Read: `M-LIM/src/dsp/LimiterEngine.h` — identify all atomic members

## Acceptance Criteria
- [ ] Run: `cd build && ctest --output-on-failure` → Expected: all tests pass
- [ ] Run: `grep -c "\.load()" M-LIM/src/dsp/LimiterEngine.cpp` → Expected: 0 (all should use explicit ordering)
- [ ] Run: `grep -c "\.store(" M-LIM/src/dsp/LimiterEngine.cpp | grep -v memory_order` → Expected: 0 (all should use explicit ordering)

## Tests
None (performance/correctness improvement, no behavior change)

## Technical Details
Search for all `.load()` and `.store()` calls (without explicit ordering) in LimiterEngine.cpp and add `std::memory_order_relaxed`. These are parameter-value atomics with no inter-thread synchronization dependencies — seq_cst ordering wastes memory barriers.

## Dependencies
None
