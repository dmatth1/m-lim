# Task 145: Extract helper methods from LoudnessMeter::updateIntegratedAndLRA()

## Description
`LoudnessMeter::updateIntegratedAndLRA()` is 138 lines (lines 190-327) performing two
conceptually separate operations:
1. **Integrated LUFS** — builds prefix sums over history, runs two-pass gated loudness algorithm
2. **LRA** — builds a histogram of short-term power, computes 10th/95th percentiles

These are independent calculations that operate on the same history data. Extract them into
two private methods:
- `float computeIntegratedLUFS() const` — returns integrated loudness in LUFS
- `float computeLRA() const` — returns loudness range in LU

`updateIntegratedAndLRA()` becomes a thin coordinator that calls both and stores results.

This makes each algorithm independently testable and easier to read.

## Produces
None

## Consumes
None

## Relevant Files
Modify: `M-LIM/src/dsp/LoudnessMeter.cpp` — extract two private methods
Modify: `M-LIM/src/dsp/LoudnessMeter.h` — declare two new private methods

## Acceptance Criteria
- [ ] Run: `awk '/^void LoudnessMeter::updateIntegratedAndLRA/,/^}/' M-LIM/src/dsp/LoudnessMeter.cpp | wc -l` → Expected: less than 20 (was 138)
- [ ] Run: `grep -n "computeIntegratedLUFS\|computeLRA" M-LIM/src/dsp/LoudnessMeter.cpp` → Expected: at least 4 hits (2 definitions + 2 call sites)
- [ ] Run: `cmake --build build -j$(nproc) 2>&1 | grep -E "error:"` → Expected: no output
- [ ] Run: `cd build && ctest --output-on-failure -R LoudnessMeter 2>&1 | tail -5` → Expected: all loudness meter tests pass

## Tests
None (existing tests provide coverage)

## Technical Details
Suggested method signatures in `LoudnessMeter.h` private section:
```cpp
float computeIntegratedLUFS() const;
float computeLRA() const;
```

The prefix sum buffer `mPrefixSums` is already a member pre-allocated in `prepare()`, so it
can be used by both helper methods without changing ownership. Since `updateIntegratedAndLRA()`
builds prefix sums first and then both algorithms use them, either:
- Keep prefix sum building in `updateIntegratedAndLRA()` before calling the helpers
- Or extract a third private `buildPrefixSums()` method

The simpler approach is to build prefix sums in `updateIntegratedAndLRA()`, then call both
helpers. Note: the prefix sum building depends on `mHistorySize` and `mPrefixSums` members,
which are valid to access from all three methods.

## Dependencies
None
