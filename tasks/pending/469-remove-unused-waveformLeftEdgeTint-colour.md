# Task 469: Remove Unused waveformLeftEdgeTint Colour Constant

## Description
`MLIMColours::waveformLeftEdgeTint` in `Colours.h` (line 35) has a comment explicitly stating it is "currently unused (removed task-423)". Dead colour constants add confusion for future workers trying to understand which colours are active.

This is a trivial cleanup — remove the single line.

## Produces
None

## Consumes
None

## Relevant Files
Modify: `M-LIM/src/ui/Colours.h` — remove line 35 (`waveformLeftEdgeTint`)

## Acceptance Criteria
- [ ] Run: `grep -r "waveformLeftEdgeTint" src/` → Expected: no output
- [ ] Run: `cmake --build build --target MLIM_Standalone_Standalone -j$(nproc) 2>&1 | tail -3` → Expected: build succeeds

## Tests
None

## Technical Details
Verify with grep that no code references `waveformLeftEdgeTint` before removing. The comment on line 35 already confirms it was removed in task-423.

## Dependencies
None
