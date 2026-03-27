# Task: Fix LRA 10th Percentile Off-By-One (EBU R128 Compliance)

## Description

The Loudness Range (LRA) computation in `LoudnessMeter::computeLRA()` uses inconsistent
comparison operators for finding the 10th and 95th histogram percentiles, causing LRA to
be systematically underestimated by up to 0.1 LU.

**File**: `M-LIM/src/dsp/LoudnessMeter.cpp`, lines 423–447

**Bug**: Line 437 uses strict `>` for the 10th percentile but line 442 uses `>=` for the
95th percentile:

```cpp
const int loTarget = static_cast<int>(0.10 * validCount);   // floor(10% * N)
const int hiTarget = static_cast<int>(0.95 * validCount);   // floor(95% * N)

...
if (!foundLo && cumul > loTarget)   // strict >  →  finds floor(10%*N)+1-th element
...
if (!foundHi && cumul >= hiTarget)  // >= is correct for floor(95%*N)-th element
```

With `cumul > loTarget` and `loTarget = floor(0.10 * N)`:
- For N=100: loTarget=10, condition triggers at cumul=11, giving ~11th percentile
- Should trigger at cumul=10, giving the true 10th percentile

Effect: `loLUFS` is shifted slightly upward (~11th rather than 10th percentile), narrowing
`LRA = hiLUFS - loLUFS` by up to one histogram bin (0.1 LU). This violates EBU R128
Tech 3342 §4.4 which specifies the 10th percentile exactly.

**Fix**: Change line 437 from `cumul > loTarget` to `cumul >= loTarget`:

```cpp
if (!foundLo && cumul >= loTarget)
```

This makes both comparisons use `>=`, both with `floor(p * N)` targets, which correctly
selects the floor(p*N)-th element as the p-th percentile.

Note: the `hiTarget` comparison (`cumul >= hiTarget`) is already correct — do NOT change it.

## Produces
None

## Consumes
None

## Relevant Files
Modify: `M-LIM/src/dsp/LoudnessMeter.cpp` — change one character on line 437

## Acceptance Criteria
- [ ] Run: `grep -n "cumul > loTarget\|cumul >= loTarget" M-LIM/src/dsp/LoudnessMeter.cpp` → Expected: shows `cumul >= loTarget` (not `cumul > loTarget`)
- [ ] Run: `cmake --build build --target MLIM_Standalone_Standalone -j$(nproc)` → Expected: compiles without errors

## Tests
None (single-character logical correction; correctness is verified by code inspection)

## Technical Details
EBU R128 Tech 3342 §4.4 requires the 10th and 95th percentile of loudness values that
pass the gating. The standard histogram-percentile convention is:
- p-th percentile = smallest bin L such that cumulative fraction ≥ p
- With integer counting: cumul ≥ floor(p * N) for both lo and hi

The fix is a single character: `>` → `>=` on line 437 only.

## Dependencies
None
