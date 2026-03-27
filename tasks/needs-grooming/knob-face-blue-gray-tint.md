# Task: Knob Face Blue-Gray Tint to Match Pro-L 2

## Description
Pro-L 2 knobs have a distinctly blue-gray face with a prominent light/dark split (light upper-left, dark lower-right), while M-LIM knobs appear more neutral gray. The reference screenshots (prol2-features.jpg, prol2-intro.jpg) show knobs with a cooler blue-gray tone, not warm/neutral gray.

Current knob face colors:
- `knobFace`: 0xff585858 (neutral gray)
- `knobFaceHighlight`: 0xffDDDDE8 (blue-tinted highlight — good)
- `knobFaceShadow`: 0xff505060 (slight blue shadow — good)

The highlight and shadow are correctly blue-tinted, but the overall composite still reads as neutral gray rather than blue-gray because the gradient spread is too tight. The Pro-L 2 knobs have a wider, more visible light-to-dark gradient with stronger blue-gray presence across the full face.

**Fix approach**: Increase the knob face gradient coverage so the blue-gray tint is more visible across the face. Shift `knobFaceShadow` slightly bluer (e.g., 0xff484860) and widen the gradient spread (increase the offset multipliers from 0.3/0.4 to 0.4/0.5). This will make the full face read as blue-gray rather than neutral gray.

## Produces
None

## Consumes
None

## Relevant Files
Modify: `src/ui/Colours.h` — adjust `knobFaceShadow` to be bluer (~0xff484860)
Modify: `src/ui/RotaryKnob.cpp` — widen gradient spread (lines 39-41): change offset multipliers from 0.3f/0.4f to 0.4f/0.5f

## Acceptance Criteria
- [ ] Run: `grep 'knobFaceShadow' src/ui/Colours.h` → Expected: shows updated color value with more blue (B > G)
- [ ] Run: `cmake --build build --target MLIM_Standalone -j$(nproc) 2>&1 | tail -3` → Expected: build succeeds

## Tests
None

## Technical Details
Reference: `/reference-docs/reference-screenshots/prol2-features.jpg` shows knobs with blue-gray faces clearly.
The gradient in RotaryKnob.cpp (lines 37-42) uses `knobFaceHighlight` → `knobFaceShadow` as a radial gradient. Widening the point spread makes the transition more gradual and visible.

## Dependencies
None
