# Task 201: Replace Hardcoded Color Literals in GainReductionMeter

## Description
`GainReductionMeter.cpp` contains at least two hardcoded ARGB color literals in `drawScale()` and nearby methods that bypass the `MLIMColours` system:

- `0xff1E1E1E` — used as a background fill (should be `MLIMColours::displayBackground`)
- `0xff1A1A1A` — used as a darker background variant (should map to an existing or new `MLIMColours` constant)

These were missed in the Task 169 colour consolidation pass. Check the entire file for any remaining hex colour literals and either map them to an existing `MLIMColours` constant or add a new named constant in `Colours.h` if no appropriate one exists.

## Produces
None

## Consumes
None

## Relevant Files
Read: `M-LIM/src/ui/Colours.h` — check existing constants before adding new ones
Modify: `M-LIM/src/ui/GainReductionMeter.cpp` — replace hex literals with named constants
Modify: `M-LIM/src/ui/Colours.h` — add any new constants needed (only if not already present)

## Acceptance Criteria
- [ ] Run: `grep -n "0xff[0-9A-Fa-f]\{6\}" M-LIM/src/ui/GainReductionMeter.cpp` → Expected: zero results
- [ ] Run: `cmake --build build -j4 2>&1 | tail -5` → Expected: build succeeds, exit 0

## Tests
None

## Technical Details
- Use `juce::Colour(0xff1E1E1E)` and compare to existing `MLIMColours` constants to find the closest match.
- If `MLIMColours::displayBackground` already equals `0xff1E1E1E`, use that. If it's slightly different, introduce `MLIMColours::kMeterBackground` or similar.
- Do not change the visual appearance — use the exact same colour value, just as a named constant.

## Dependencies
None
