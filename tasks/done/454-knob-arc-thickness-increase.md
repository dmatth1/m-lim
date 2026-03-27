# Task 454: Increase Knob Value Arc Thickness for Pro-L 2 Parity

## Description
Pro-L 2 knobs have a more prominent blue value arc that's visibly thicker than M-LIM's current 2.5px stroke. In the reference close-ups (prol2-features.jpg), the blue arc around the knob face is approximately 3.5-4px wide and uses a slightly brighter blue than M-LIM's current `knobArc` (0xff6898C8).

Current implementation in `RotaryKnob.cpp`:
- Track arc: `panelBorder` (0xff333333), 2.5px stroke (line 55)
- Value arc: `knobArc` (0xff6898C8), 2.5px stroke (line 74)

Both arcs should be slightly thicker (3.0px) to match the Pro-L 2 visual weight.

**Fix approach**: Increase arc stroke width from 2.5f to 3.0f for both track and value arcs. Optionally brighten `knobArc` slightly to ~0xff70A0D0 for better visibility.

## Produces
None

## Consumes
None

## Relevant Files
Modify: `src/ui/RotaryKnob.cpp` — change stroke width from 2.5f to 3.0f on lines 55 and 74
Modify: `src/ui/Colours.h` — optionally adjust `knobArc` to ~0xff70A0D0 for brighter blue

## Acceptance Criteria
- [ ] Run: `grep -n 'PathStrokeType' src/ui/RotaryKnob.cpp` → Expected: shows 3.0f stroke width
- [ ] Run: `cmake --build build --target MLIM_Standalone -j$(nproc) 2>&1 | tail -3` → Expected: build succeeds

## Tests
None

## Technical Details
The track arc and value arc share the same stroke width. Changing both to 3.0f maintains proportionality while increasing visibility. Reference: `/reference-docs/reference-screenshots/prol2-features.jpg`.

## Dependencies
Requires task 453
