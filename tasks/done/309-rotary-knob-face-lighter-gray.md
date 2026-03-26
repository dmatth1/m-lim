# Task 309: Rotary Knob Face — Lighter Gray to Match Pro-L 2 Reference

## Description
The rotary knobs in the control strip are currently rendered with a dark blue-gray face:
- `knobFaceHighlight = 0xff7080A0` = rgb(112, 128, 160)
- `knobFaceShadow    = 0xff303448` = rgb(48, 52, 72)
- Average face colour ≈ `#505A74` — very dark blue

The Pro-L 2 reference shows much lighter gray knobs:
- Reference knob face centre: `#BCBCC6` = rgb(188, 188, 198) — near-white light gray
- Reference knob face edge: `#74798B` = rgb(116, 121, 139) — medium neutral gray-blue

This is a large colour gap (>100 units in each channel at the face centre) that directly
contributes to the control strip RMSE (22.56%). The knobs are the most visually prominent
elements of the control strip.

**Fix in `Colours.h`:**
```cpp
// BEFORE:
const juce::Colour knobFaceHighlight { 0xff7080A0 };
const juce::Colour knobFaceShadow   { 0xff303448 };

// AFTER:
const juce::Colour knobFaceHighlight { 0xffCCCCD6 };  // near-white light gray highlight
const juce::Colour knobFaceShadow   { 0xff707888 };  // medium neutral gray shadow
```

The `knobFace` constant (`0xff4A526A`) is not directly used in `RotaryKnob::paint()` (the face
uses only `knobFaceHighlight` and `knobFaceShadow`) but update it for consistency:
```cpp
const juce::Colour knobFace { 0xff9A9AA8 };  // medium gray (was 0xff4A526A)
```

Also adjust the track/arc background to better match reference. The reference knob ring track
appears as a medium-dark ring. Current `panelBorder = 0xff333333` is used for the track — this
may need adjustment in RotaryKnob.cpp or via a dedicated constant.

## Produces
None

## Consumes
None

## Relevant Files
Modify: `M-LIM/src/ui/Colours.h` — change `knobFaceHighlight`, `knobFaceShadow`, `knobFace`
Read: `M-LIM/src/ui/RotaryKnob.cpp` — verify which colour constants are used where
Read: `/reference-docs/video-frames/v1-0009.png` — reference knob appearance

## Acceptance Criteria
- [ ] Run: `cmake --build /workspace/M-LIM/build --config Release -j$(nproc) --target MLIM_Standalone 2>&1 | tail -3` → Expected: exit 0
- [ ] Visual: Launch standalone, capture screenshot. Control strip knobs should appear as light
  gray (near-white face with darker gray edge), clearly more similar to Pro-L 2's appearance.
- [ ] Visual: Pointer/tick line on knob should still be clearly visible (white pointer against
  lighter gray background — may need to verify contrast is maintained).

## Tests
None

## Technical Details
`RotaryKnob::paint()` uses:
- `MLIMColours::knobFaceHighlight` for the bright spot of the 3D gradient
- `MLIMColours::knobFaceShadow` for the dark spot

By changing to `#CCCCCD6` (bright) and `#707888` (shadow), the face will appear as a medium-to-light
gray circle. The white pointer (`knobPointer = 0xffFFFFFF`) will still be visible against the
lighter background.

**Note:** If the white pointer (`#FFFFFF`) disappears against the lighter face, consider
changing `knobPointer` to a dark charcoal (`0xff2A2A2A`) or keeping it white and adjusting the
face slightly darker (e.g., `knobFaceHighlight = 0xffB8B8C4`).

## Dependencies
None
