# Task 465: Fix LoudnessMeter computeLRA() Overwriting Shared mWindowPowers Buffer

## Description
In `LoudnessMeter.cpp`, `updateIntegratedAndLRA()` (line 240) calls `computeIntegratedLUFS()` then `computeLRA()` sequentially. Both methods write into `mWindowPowers` — a shared pre-allocated buffer:

- `computeIntegratedLUFS()` fills it with 400ms window powers (line 292)
- `computeLRA()` fills it with 3s window powers (line 381)

Currently this works because the methods are called sequentially and neither reads the other's data. However, this is a latent coupling:
1. If either method is called independently (e.g., for separate UI readouts), the other's data is stale
2. The shared mutable state makes the code harder to reason about
3. `computeLRA()` could use a local stack array or a second pre-allocated buffer to be independent

This is low-priority but worth documenting the coupling or decoupling the buffers.

## Produces
None

## Consumes
None

## Relevant Files
Modify: `M-LIM/src/dsp/LoudnessMeter.h` — add a second pre-allocated buffer `mLraWindowPowers` (or add a comment documenting the coupling)
Modify: `M-LIM/src/dsp/LoudnessMeter.cpp` — `computeLRA()` uses `mLraWindowPowers` instead of `mWindowPowers`

## Acceptance Criteria
- [ ] Run: `cmake --build build --target MLIM_Standalone_Standalone -j$(nproc) 2>&1 | tail -3` → Expected: build succeeds
- [ ] Run: `cd build && ctest --output-on-failure` → Expected: all tests pass

## Tests
None (existing loudness meter accuracy tests cover the behavior)

## Technical Details
The simplest fix is to add `std::vector<double> mLraWindowPowers;` allocated in `prepare()` to `kMaxHistoryBlocks` size, and use it in `computeLRA()`. This decouples the two methods so they can be called independently without silent data corruption.

Alternatively, if the coupling is intentional and will never change, add a clear comment at the top of both methods: "// NOTE: shares mWindowPowers with computeIntegratedLUFS — must be called sequentially from updateIntegratedAndLRA()".

## Dependencies
None
