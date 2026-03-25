# Task 220: Replace Hardcoded Color Literals in TopBar.cpp and RotaryKnob.cpp

## Description
Two UI files still contain hardcoded ARGB hex literals that bypass the `MLIMColours` system. These were missed in the Task 169 colour consolidation pass.

**TopBar.cpp** (4 literals):
- `0xff232323` (line ~17) — preset label background. Closest match: `MLIMColours::background` (0xff1E1E1E). Since this is slightly lighter, add a new constant `MLIMColours::topBarPresetBackground { 0xff232323 }` to Colours.h, or map to `MLIMColours::widgetBackground` (0xff2A2A2A) if the distinction isn't important.
- `0xff2A2A2A` (line ~67 in `styleBarButton`) — button off background. Maps to `MLIMColours::widgetBackground`.
- `0xff3A3A3A` (line ~68 in `styleBarButton`) — button pressed/on background. Add new constant `MLIMColours::buttonPressedBackground { 0xff3A3A3A }` or map to the closest existing constant.
- `0xff1A1A1A` (line ~77 in `paint()`) — bar fill background. Maps to `MLIMColours::peakLabelBackground`.

**RotaryKnob.cpp** (1 literal):
- `0xff555555` (line ~65) — used for the knob arc/track when disabled or at minimum. No exact match in Colours.h; add `MLIMColours::knobArcDim { 0xff555555 }` (dim grey for inactive arc) or check if this should use `MLIMColours::textSecondary` (0xff9E9E9E — lighter than 555555).

## Produces
None

## Consumes
None

## Relevant Files
Read: `M-LIM/src/ui/Colours.h` — check existing constants before adding new ones
Modify: `M-LIM/src/ui/TopBar.cpp` — replace 4 hex literals with named constants
Modify: `M-LIM/src/ui/RotaryKnob.cpp` — replace 1 hex literal with named constant
Modify: `M-LIM/src/ui/Colours.h` — add any new constants needed (only if not already present)

## Acceptance Criteria
- [ ] Run: `grep -n "0xff[0-9A-Fa-f]\{6\}" M-LIM/src/ui/TopBar.cpp` → Expected: zero results
- [ ] Run: `grep -n "0xff[0-9A-Fa-f]\{6\}" M-LIM/src/ui/RotaryKnob.cpp` → Expected: zero results
- [ ] Run: `cmake --build build -j4 2>&1 | tail -5` → Expected: build succeeds, exit 0
- [ ] Run: `cd build && ctest --output-on-failure` → Expected: all tests pass

## Tests
None (colour alias change only)

## Technical Details
Map or add constants in `Colours.h` in the appropriate section:
- `0xff2A2A2A` → already `MLIMColours::widgetBackground`
- `0xff1A1A1A` → already `MLIMColours::peakLabelBackground`
- `0xff232323` → add `topBarPresetBackground { 0xff232323 }` in the "Background colours" block, or use `background` if the 5-shade difference is acceptable
- `0xff3A3A3A` → add to the "Button colours" block as `buttonPressedBackground { 0xff3A3A3A }` (this is the active/highlighted state)
- `0xff555555` → add to the "Knob colours" block as `knobArcDim { 0xff555555 }`

## Dependencies
None
