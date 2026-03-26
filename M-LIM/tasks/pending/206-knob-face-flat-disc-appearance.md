# Task 206: Rotary Knob Face — Flat Silver-Grey Disc Matching Reference

## Description
The current RotaryKnob renders its face with a 3D spherical gradient (`knobFaceHighlight` → `knobFaceShadow`), giving a pronounced ball/dome appearance. The FabFilter Pro-L 2 reference shows **flat, matte silver-grey disc** knobs with:
- Uniform flat face fill (no gradient or very subtle radial highlight)
- Thin white/light pointer line from centre to edge
- Arc track in a subdued grey
- Slightly larger effective diameter relative to the slot

### Visual target (from prol2-main-ui.jpg, prol2-features.jpg):
- Knob face: uniform mid-grey (#808080 range), possibly with very slight radial highlight (~10% brighter at top-left)
- Pointer: thin white line, extends from center outward ~60% of radius
- Track arc: very thin dark grey, barely visible
- Value arc: medium blue, visible but not thick
- No strong 3D shading

### Required Change:
In `RotaryKnob.cpp`:
1. Replace the `ColourGradient` sphere fill with a simple flat fill using `MLIMColours::knobFace` (or a slightly adjusted grey)
2. Optionally keep a very subtle 2-colour radial gradient (max 20% brightness difference)
3. Reduce pointer thickness to ~1.5px
4. Ensure the value arc remains clearly visible (medium blue, 2px stroke)

In `Colours.h`:
- `knobFace` is currently `0xff4A526A` (steel blue-grey) — can be adjusted toward `0xff7A7A7A` (neutral grey) for flatter look
- `knobFaceHighlight` and `knobFaceShadow` may be retired if gradient is removed

## Produces
None

## Consumes
None

## Relevant Files
Modify: `src/ui/RotaryKnob.cpp` — simplify face rendering, reduce 3D gradient, slim pointer
Modify: `src/ui/Colours.h` — adjust knobFace, knobFaceHighlight, knobFaceShadow values
Read: `/reference-docs/reference-screenshots/prol2-features.jpg` — close-up of knob appearance
Read: `/reference-docs/reference-screenshots/prol2-main-ui.jpg` — full layout knob appearance

## Acceptance Criteria
- [ ] Run: `cmake --build build --target MLIM_Standalone -j$(nproc) 2>&1 | tail -3` → Expected: `[100%] Built target MLIM_Standalone`
- [ ] Run: visual inspection — knobs appear as flatter grey discs, not shiny 3D spheres; pointer is thin white line

## Tests
None

## Technical Details
- Replace `juce::ColourGradient` fill with: `g.setColour(MLIMColours::knobFace); g.fillEllipse(...)`
- Optional very subtle highlight: a tiny ellipse offset top-left, filled with `knobFace.brighter(0.15f)`, alpha ~0.4
- Pointer: reduce to `1.5f` thickness, keep white colour
- Track arc stroke: keep at `1.5f`
- Value arc stroke: keep at `2.0f`

## Dependencies
None
