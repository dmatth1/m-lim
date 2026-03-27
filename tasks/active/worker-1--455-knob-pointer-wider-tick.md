# Task 455: Widen Knob Pointer Tick to Match Pro-L 2

## Description
Pro-L 2 knobs have a wider, more visible white pointer/indicator mark. In the reference screenshots (prol2-features.jpg), the pointer is clearly a wide white slash that's easy to see at a glance. M-LIM's pointer is a thin 2.5px rectangle that's barely visible, especially at small knob sizes.

Current implementation in `RotaryKnob.cpp` lines 86-96:
- `pointerThickness = 2.5f` (too thin)
- `pointerLength = faceRadius * 0.50f` (good)
- Drawn as a simple rectangle

**Fix approach**: Increase pointer thickness to 3.5px and optionally taper it slightly (wider at the edge, narrower at center) to match the Pro-L 2 look. The pointer should be drawn from the inner face edge outward toward the knob rim, not from center outward.

## Produces
None

## Consumes
None

## Relevant Files
Modify: `src/ui/RotaryKnob.cpp` — increase `pointerThickness` from 2.5f to 3.5f (line 87), adjust rectangle origin to start at `-faceRadius` and extend to `-faceRadius + pointerLength` (line 90-91)

## Acceptance Criteria
- [ ] Run: `grep 'pointerThickness' src/ui/RotaryKnob.cpp` → Expected: shows 3.5f (or similar increase)
- [ ] Run: `cmake --build build --target MLIM_Standalone -j$(nproc) 2>&1 | tail -3` → Expected: build succeeds

## Tests
None

## Technical Details
Reference: `/reference-docs/reference-screenshots/prol2-features.jpg` — close-up of knobs shows wide white pointer.
The pointer rectangle is centered on the rotation axis and rotated to the current angle. Currently starts at `-faceRadius` and has length `faceRadius * 0.5`, which means it extends from face edge to halfway toward center. This direction is correct but the thickness needs increase.

## Dependencies
Requires task 454
