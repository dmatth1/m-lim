# Task 432: Fix LRA 10th Percentile Off-By-One (EBU R128 Compliance)

## Description
`LoudnessMeter::computeLRA()` in `src/dsp/LoudnessMeter.cpp` (~line 437) uses inconsistent
comparison operators for the 10th and 95th histogram percentiles:

```cpp
if (!foundLo && cumul > loTarget)   // strict >  →  finds (floor(10%*N)+1)-th element
...
if (!foundHi && cumul >= hiTarget)  // >= is correct
```

With `cumul > loTarget` and `loTarget = floor(0.10 * N)`:
- For N=100: loTarget=10, condition triggers at cumul=11 → reports ~11th percentile
- Should trigger at cumul=10 → true 10th percentile

This violates EBU R128 Tech 3342 §4.4 and causes LRA to be underestimated by up to 0.1 LU.

Fix: change `>` to `>=` on the `foundLo` line:
```cpp
if (!foundLo && cumul >= loTarget)  // was: >
```

## Produces
None

## Consumes
None

## Relevant Files
Modify: `src/dsp/LoudnessMeter.cpp` — line ~437, change `>` to `>=` for `foundLo` check

## Acceptance Criteria
- [ ] Run: `grep -n "foundLo" src/dsp/LoudnessMeter.cpp` → Expected: both `foundLo` lines use `>=`
- [ ] Run: `cmake --build build --target MLIM_Standalone_Standalone -j$(nproc) 2>&1 | tail -3` → Expected: build succeeds, exit 0

## Tests
None

## Technical Details
This is a single-character fix. The line at ~437 currently reads:
```cpp
if (!foundLo && cumul > loTarget)
```
Change to:
```cpp
if (!foundLo && cumul >= loTarget)
```

Build Standalone: `cmake --build build --target MLIM_Standalone_Standalone -j$(nproc)`.

## Dependencies
None
