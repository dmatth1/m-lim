# Task 197: Fix LoudnessMeter Non-Atomic History Index Race

## Description
`LoudnessMeter` has two plain `int` members — `mHistoryHead` and `mHistorySize` — that are written on the audio thread (`processBlock`) and read on the UI thread (via `getIntegratedLUFS()` and `getLoudnessRange()`). Because the two assignments are non-atomic, the UI thread can observe a torn state where one has been updated but not the other.

Additionally, `mWindowPowers` and `mPrefixSums` are declared `mutable` and mutated inside `const` methods (`computeIntegratedLUFS()` and `computeLRA()`). These are working scratch buffers that the UI thread can be filling at the same time the audio thread calls `processBlock()`, which is a data race.

Fix both issues:
1. Change `mHistoryHead` and `mHistorySize` to `std::atomic<int>` in the header and use appropriate memory ordering at write sites (audio thread: `relaxed` store is fine; read sites can also use `relaxed`).
2. Convert `mWindowPowers` and `mPrefixSums` from `mutable` members to local variables inside `computeIntegratedLUFS()` and `computeLRA()`. They are only used within those functions, so they do not need to be members at all.

## Produces
None

## Consumes
None

## Relevant Files
Read: `M-LIM/src/dsp/LoudnessMeter.h` — locate mHistoryHead, mHistorySize, mWindowPowers, mPrefixSums declarations
Modify: `M-LIM/src/dsp/LoudnessMeter.h` — change ints to `std::atomic<int>`, remove mutable vector members
Modify: `M-LIM/src/dsp/LoudnessMeter.cpp` — update all read/write sites for atomic ints; move vector declarations to local scope in compute functions

## Acceptance Criteria
- [ ] Run: `grep -n "mHistoryHead\|mHistorySize" M-LIM/src/dsp/LoudnessMeter.h` → Expected: both declared as `std::atomic<int>`
- [ ] Run: `grep -n "mutable" M-LIM/src/dsp/LoudnessMeter.h` → Expected: no `mutable` keyword present
- [ ] Run: `cmake --build build -j4 2>&1 | tail -5` → Expected: build succeeds, exit 0
- [ ] Run: `cd build && ctest -R loudness --output-on-failure 2>&1 | tail -10` → Expected: all loudness meter tests pass

## Tests
None (the race is a data race, not a logic error; existing tests cover correctness)

## Technical Details
- `mWindowPowers` and `mPrefixSums` are only used as scratch; declare them as `std::vector<double>` locals inside `computeIntegratedLUFS()` and `computeLRA()`.
- Atomic history indices: replace `mHistoryHead` write with `mHistoryHead.store(newVal, std::memory_order_relaxed)`. Reads in `computeIntegratedLUFS()` / `computeLRA()` use `mHistoryHead.load(std::memory_order_relaxed)`.

## Dependencies
None
