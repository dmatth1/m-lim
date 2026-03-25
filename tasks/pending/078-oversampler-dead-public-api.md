# Task 078: Oversampler Has Three Dead Public Methods

## Description
`Oversampler` exposes three public methods that are never called anywhere in
the codebase:

- `requestFactor(int pendingFactor)` — stores a pending factor without applying it
- `needsRebuild() const` — checks if a rebuild is pending
- `commitRebuild()` — applies the pending factor and rebuilds

These methods were intended as an alternative deferred-rebuild API (the caller
checks `needsRebuild()` and calls `commitRebuild()` from a non-RT thread).
However, the actual deferred rebuild mechanism is implemented one level up, in
`LimiterEngine` + `PluginProcessor` via `mDeferredOversamplingChange`,
`mOversamplingChangePending`, and `handleAsyncUpdate()`. The `Oversampler`-level
API is entirely superseded and never used.

Dead public API creates confusion: a reader of `Oversampler.h` might assume these
methods are part of the required call sequence when changing oversampling factors,
leading to incorrect usage. The methods also maintain a `mPendingFactor` atomic
that is written by `requestFactor` but only read by `needsRebuild`/`commitRebuild`
(and synced needlessly in `setFactor`).

## Produces
None

## Consumes
None

## Relevant Files
Modify: `M-LIM/src/dsp/Oversampler.h` — remove `requestFactor`, `needsRebuild`,
  `commitRebuild` declarations
Modify: `M-LIM/src/dsp/Oversampler.cpp` — remove `requestFactor`, `needsRebuild`,
  `commitRebuild` implementations; remove `mPendingFactor` sync line in
  `setFactor()` (line ~18: `mPendingFactor.store(factor);`); remove `mPendingFactor`
  from `setFactor`.
Modify: `M-LIM/src/dsp/Oversampler.h` — remove `std::atomic<int> mPendingFactor`
  member if no callers remain.

## Acceptance Criteria
- [ ] Run: `grep -n "requestFactor\|needsRebuild\|commitRebuild\|mPendingFactor" M-LIM/src/dsp/Oversampler.h M-LIM/src/dsp/Oversampler.cpp` → Expected: no output.
- [ ] Run: `grep -rn "requestFactor\|needsRebuild\|commitRebuild" M-LIM/src/` → Expected: no output.
- [ ] Run: `cd M-LIM && cmake --build build -j$(nproc) 2>&1 | tail -3` → Expected: builds without errors.

## Tests
None — dead code removal; no behavioral change.

## Technical Details
- `setFactor()` in `Oversampler.cpp` has `mPendingFactor.store(factor);` — this
  line can be removed since nothing reads `mPendingFactor` after removal of the
  three methods.
- Verify with grep that no test or other file calls these methods before deleting.

## Dependencies
None
