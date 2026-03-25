# Task 213: Replace push_back() with index-based access in LoudnessMeter audio thread path

## Description
`LoudnessMeter::computeIntegratedLUFS()` runs on the audio thread (called from `processBlock()` via `updateIntegratedAndLRA()` every ~1 s). It currently uses the pattern:

```cpp
mWindowPowers.clear();
for (int i = 0; i < numWin400; ++i)
    mWindowPowers.push_back(sum / kMomentaryBlocks);
```

While `prepare()` calls `mWindowPowers.reserve(kMaxHistoryBlocks)` which prevents allocation in normal operation (because `numWin400 <= kMaxHistoryBlocks - 3`), `push_back()` is semantically an allocating call. Its no-allocation guarantee is implicit and relies on a distant relationship between `numWin400`, `mHistorySize`, and the reserve amount. Any future change to the history bounds or window calculations could silently reintroduce heap allocation on the audio thread.

Replace the `reserve()` + `push_back()` pattern with a `resize(kMaxHistoryBlocks)` in `prepare()` followed by direct index assignment in `computeIntegratedLUFS()`, making the no-allocation guarantee explicit and unconditional.

## Produces
None

## Consumes
None

## Relevant Files
Read: `src/dsp/LoudnessMeter.h` — `mWindowPowers` declaration; `kMaxHistoryBlocks` constant
Modify: `src/dsp/LoudnessMeter.cpp` — change `reserve()` to `resize()` in `prepare()`, change `push_back()` to indexed assignment in `computeIntegratedLUFS()`

## Acceptance Criteria
- [ ] Run: `grep -n "push_back" src/dsp/LoudnessMeter.cpp` → Expected: no matches (push_back removed)
- [ ] Run: `grep -n "reserve" src/dsp/LoudnessMeter.cpp` → Expected: no matches (reserve removed; replaced by resize)
- [ ] Run: `cmake --build build -j4 2>&1 | tail -5` → Expected: build succeeds with 0 errors
- [ ] Run: `cd build && ctest -R loudness --output-on-failure` → Expected: all loudness meter tests pass

## Tests
None (this is a refactor with no behavioural change — existing tests verify correctness)

## Technical Details

In `prepare()`, change:
```cpp
mWindowPowers.clear();
mWindowPowers.reserve(static_cast<size_t>(kMaxHistoryBlocks));
```
to:
```cpp
mWindowPowers.assign(static_cast<size_t>(kMaxHistoryBlocks), 0.0);
```

In `computeIntegratedLUFS()`, change:
```cpp
mWindowPowers.clear();
const int numWin400 = n - kMomentaryBlocks + 1;
for (int i = 0; i < numWin400; ++i)
{
    const double sum = ...;
    mWindowPowers.push_back(sum / kMomentaryBlocks);
}
```
to:
```cpp
const int numWin400 = n - kMomentaryBlocks + 1;
for (int i = 0; i < numWin400; ++i)
{
    const double sum = ...;
    mWindowPowers[static_cast<size_t>(i)] = sum / kMomentaryBlocks;
}
```
All existing loops that iterate `mWindowPowers` already use range-based `for (double p : mWindowPowers)` — these must be updated to iterate only `[0, numWin400)` since the vector is now permanently sized at `kMaxHistoryBlocks`. Pass `numWin400` explicitly or use a `std::span`/raw loop with `numWin400` as the bound.

Note: `mWindowPowers` is declared `mutable` in the header; no header change is needed.

## Dependencies
None
