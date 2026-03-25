# Task 074: Knob Face Color and 3D Gradient for Pro-L 2 Parity

## Description
The current knob face colour (`knobFace: 0xff3A3A3A`) is a neutral dark grey that doesn't match the Pro-L 2 reference. Pixel sampling of the reference image shows knob face colours in the range `#626F8C`–`#7F88A9` — a medium **steel blue-grey**. Additionally, Pro-L 2 knobs have a subtle 3D radial gradient (lighter at top-left, darker at bottom-right) creating a sphere/dome illusion. The current flat fill looks noticeably flat by comparison.

Reference pixel samples from `/reference-docs/reference-screenshots/prol2-main-ui.jpg` at knob face positions:
- `(320+1000)`: `#7F88A9`
- `(330+1000)`: `#7F88A9`
- `(310+1020)`: `#393E52` (darker lower-edge of knob)
- `(310+960)`: `#7F88A9` → `#939CBB` (highlight at top)

Reference: `/reference-docs/reference-screenshots/prol2-features.jpg` (close-up of knobs on blue-grey background), `/reference-docs/video-frames/v1-0020.png`.

## Produces
None

## Consumes
ColoursDefinition
LookAndFeelDefinition

## Relevant Files
Modify: `M-LIM/src/ui/Colours.h` — update `knobFace` colour to steel blue-grey
Modify: `M-LIM/src/ui/LookAndFeel.cpp` — add radial gradient to `drawRotarySlider` knob face fill
Modify: `M-LIM/src/ui/RotaryKnob.cpp` — update matching colour in paint() knob face fill

## Acceptance Criteria
- [ ] Run: `cd M-LIM && cmake --build build --target MLIM_VST3 -j$(nproc) 2>&1 | tail -3` → Expected: builds successfully
- [ ] Run: `grep "knobFace" M-LIM/src/ui/Colours.h` → Expected: colour value is in the `#505872` to `#606878` range (NOT `#3A3A3A`)
- [ ] Run: `grep -c "radialGradient\|ColourGradient\|highlight\|withMultipliedAlpha" M-LIM/src/ui/LookAndFeel.cpp` → Expected: at least 1 match (gradient being drawn)

## Tests
None (visual component — verified by UI parity auditor)

## Technical Details

**Colour change:**
- `knobFace`: change from `0xff3A3A3A` to `0xff505872` (medium steel blue-grey)
- Optionally add `knobFaceHighlight` constant `0xff7080A0` for the gradient highlight

**3D gradient approach:**
In `drawRotarySlider`, replace the flat `g.fillEllipse(knobFace)` with a radial gradient:
```cpp
// 3D sphere gradient on knob face
juce::ColourGradient gradient(
    MLIMColours::knobFaceHighlight,   // bright spot top-left  ~#7080A0
    centreX - faceRadius * 0.3f,      // highlight x offset
    centreY - faceRadius * 0.3f,      // highlight y offset
    MLIMColours::knobFaceShadow,      // dark at bottom-right ~#303448
    centreX + faceRadius * 0.4f,
    centreY + faceRadius * 0.4f,
    true  // isRadial
);
g.setGradientFill(gradient);
g.fillEllipse(centreX - faceRadius, centreY - faceRadius,
              faceRadius * 2.0f, faceRadius * 2.0f);
```

**Also fix knob face radius inconsistency:**
- `LookAndFeel::drawRotarySlider` uses `faceRadius = radius * 0.85f`
- `RotaryKnob::paint()` uses `faceRadius = radius * 0.78f`
- These must match. The correct Pro-L 2 ratio leaves visible graduation marks around the edge — use `0.78f` consistently in both places.

**New colour constants to add to `Colours.h`:**
```cpp
const juce::Colour knobFace         { 0xff505872 };  // steel blue-grey (was 3A3A3A)
const juce::Colour knobFaceHighlight{ 0xff7080A0 };  // knob highlight for 3D gradient
const juce::Colour knobFaceShadow   { 0xff303448 };  // knob shadow for 3D gradient
```

## Dependencies
Requires task 003
