# Task 307: RotaryKnob — Neutral Gray Face, Subdued Value Arc

## Description
The reference Pro-L 2 knobs (v1-0030, v1-0040, prol2-features.jpg) have a **neutral silver-gray** metallic face with a subtle or near-invisible colored arc. The arc appears as a faint brighter tone — not a prominent colored band. In particular, the knob face looks more silver/light-gray, not the current steel blue-gray (`knobFace = 0xff4A526A`).

M-LIM knobs currently use:
- Face: `knobFace = 0xff4A526A` (steel blue-gray tint) — too blue, should be more neutral
- Value arc: `knobArc = 0xff4888C8` (medium blue, 2.5px) — too prominent, reference arc is more subdued
- Track arc: `panelBorder = 0xff333333` (dark gray) — reasonable but could be slightly lighter

## Fix

In `Colours.h`:
1. Change `knobFace` from `0xff4A526A` to `0xff585858` (neutral medium gray, matches Pro-L 2 silver)
2. Change `knobFaceHighlight` from `0xff7080A0` to `0xff808080` (neutral light gray highlight)
3. Change `knobFaceShadow` from `0xff303448` to `0xff303030` (neutral dark shadow)
4. Change `knobArc` from `0xff4888C8` to `0xff6898C8` (slightly lighter blue, more subtle at 2.5px)
5. Change `knobArcDim` from `0xff555555` to `0xff404040` (slightly darker dim track for better contrast with face)

Also in `RotaryKnob.cpp`, reduce the value arc stroke width from `2.5f` to `2.0f` to make it less prominent.

## Produces
None

## Consumes
None

## Relevant Files
Modify: `M-LIM/src/ui/Colours.h` — update knob color constants
Modify: `M-LIM/src/ui/RotaryKnob.cpp` — reduce arc stroke width
Read: `M-LIM/src/ui/RotaryKnob.cpp` — verify paint() logic for arc drawing

## Acceptance Criteria
- [ ] Run: `cmake --build M-LIM/build --config Release -j$(nproc) 2>&1 | tail -5` → Expected: build succeeds, exit 0
- [ ] Run: launch app, inspect control strip knobs → Expected: knobs appear more silver/neutral gray (less blue tint) with a subtler blue arc
- [ ] Run: `source Scripts/ui-test-helper.sh && start_app && screenshot screenshots/task-307-after.png && stop_app` → Expected: screenshot saved
- [ ] Compare task-307-after.png control strip against `/reference-docs/reference-screenshots/prol2-features.jpg` → Expected: knob face color closer to silver-gray reference

## Tests
None

## Technical Details
- Only `Colours.h` constants and one stroke width change — no logic changes
- The arc coloring in `RotaryKnob.cpp` uses `MLIMColours::knobArc` for value arc and `MLIMColours::panelBorder` for track arc — both are set via Colours.h
- Knob face uses `knobFaceHighlight` → `knobFaceShadow` radial gradient — changing to neutral gray removes the blue tint

## Dependencies
None (can run in parallel with tasks 304, 305, 306)
