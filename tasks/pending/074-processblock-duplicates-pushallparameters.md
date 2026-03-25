# Task 074: processBlock Duplicates pushAllParametersToEngine Verbatim

## Description
`PluginProcessor::processBlock()` contains a verbatim copy of the entire
`pushAllParametersToEngine()` body (20+ parameter setters). The two blocks
are byte-for-byte identical today, but will silently diverge the moment
a developer adds a parameter and updates only one of them. This has already
happened once: any future parameter added to `pushAllParametersToEngine` must
also be manually added to the matching block in `processBlock` — there is nothing
to enforce consistency.

`processBlock` (lines 100–118 in `PluginProcessor.cpp`) should simply call
`pushAllParametersToEngine()`. The helper already reads from the same cached
raw-pointer fields and is declared private, so the refactor is purely mechanical.

## Produces
None

## Consumes
None

## Relevant Files
Modify: `M-LIM/src/PluginProcessor.cpp` — replace the duplicate setter block in
  `processBlock` with a call to `pushAllParametersToEngine()`
Read: `M-LIM/src/PluginProcessor.h` — `pushAllParametersToEngine` is `private`,
  only called on the audio thread — this is fine.

## Acceptance Criteria
- [ ] Run: `grep -c "limiterEngine\.set" M-LIM/src/PluginProcessor.cpp` → Expected: output is the count from `pushAllParametersToEngine` only (not doubled). That is, the result should equal 20 (one copy), not 40 (two copies).
- [ ] Run: `cd M-LIM && cmake --build build -j$(nproc) 2>&1 | tail -3` → Expected: `Built target` with no errors.

## Tests
None — pure refactor; existing integration tests cover behavior.

## Technical Details
Replace lines 100–118 of `PluginProcessor.cpp` (the entire `if (pXxx)` block
inside `processBlock`) with a single call:
```cpp
pushAllParametersToEngine();
```
The NOTE comment on line 120 about oversamplingFactor should move to be adjacent
to that call for clarity.

## Dependencies
None
