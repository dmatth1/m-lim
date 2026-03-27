# Task 381: Brighten Rotary Knob Face to Match Reference Silver Appearance

## Description

Pixel analysis of the control strip shows M-LIM knobs are significantly darker than
the reference FabFilter Pro-L 2 knobs:
- Reference knob highlight area: `#DBDBE4` (R=219, G=219, B=228) — bright silver
- M-LIM knob at same position: `#7F7F88` (R=127, G=127, B=136) — medium gray

The `knobFaceHighlight = 0xffC0C0D0` is close to reference R=219, but the rendered gradient
output is dimmer than expected. The `knobFaceShadow = 0xff303030` (R=48) is very dark,
pulling the overall appearance dark.

**Fix:**
- `knobFaceHighlight`: `0xffC0C0D0` → `0xffDDDDE8`  (brighter blue-tinted highlight, ≈ ref #DBDBE4)
- `knobFaceShadow`:    `0xff303030` → `0xff505060`  (lighter shadow — raises mid-tones)

**Why mid-tones matter:** The radial gradient interpolates from `knobFaceHighlight` (center)
to `knobFaceShadow` (outer edge). With shadow at #303030, mid-tone ≈ R=120; reference mid-tones
are R=152. Raising shadow to #505060 raises mid-tones proportionally.

**Expected RMSE gain:** Control strip −0.4 to −0.6pp.

**Note:** Task 378 also modifies `Colours.h` (different constant: displayGradientTop).
These can run in parallel; each worker edits only their specified constants.

## Produces
None

## Consumes
None

## Relevant Files
Modify: `src/ui/Colours.h` — `knobFaceHighlight`, `knobFaceShadow` constants
Read: `src/ui/RotaryKnob.cpp` — confirms gradient usage in `paint()`

## Acceptance Criteria
- [ ] Run: build + screenshot + control strip RMSE → Expected: < wave-17 control baseline
- [ ] Run: full image RMSE → Expected: ≤ wave-17 full baseline (no regression)
- [ ] Run: visual check → Expected: knobs appear brighter/more metallic silver with clear highlight

## Tests
None

## Technical Details

In `src/ui/Colours.h`, change two constants:
```cpp
// Before:
const juce::Colour knobFaceHighlight{ 0xffC0C0D0 };
const juce::Colour knobFaceShadow   { 0xff303030 };

// After:
const juce::Colour knobFaceHighlight{ 0xffDDDDE8 };  // brighter blue-tinted highlight
const juce::Colour knobFaceShadow   { 0xff505060 };  // lighter shadow
```

Note: `knobFace` constant in `Colours.h` is NOT used in `RotaryKnob.cpp::paint()` —
only `knobFaceHighlight` and `knobFaceShadow` are used in the gradient fill.

Build and measure:
```bash
export CCACHE_DIR=/build-cache
cmake --build /workspace/M-LIM/build --target MLIM_Standalone_Standalone -j$(nproc)

convert /reference-docs/reference-screenshots/prol2-main-ui.jpg \
    -crop 1712x1073+97+32 +repage -resize 900x500! /tmp/ref.png

pkill -f "Standalone/M-LIM" 2>/dev/null; sleep 1
DISPLAY=:99 /workspace/M-LIM/build/MLIM_artefacts/Release/Standalone/M-LIM &>/dev/null &
sleep 8
DISPLAY=:99 scrot /tmp/raw.png
pkill -f "Standalone/M-LIM"

convert /tmp/raw.png -crop 908x500+509+325 +repage -resize 900x500! /tmp/mlim.png

convert /tmp/mlim.png -crop 900x90+0+410 +repage /tmp/ctrl.png
convert /tmp/ref.png  -crop 900x90+0+410 +repage /tmp/ref_ctrl.png
compare -metric RMSE /tmp/ref_ctrl.png /tmp/ctrl.png /dev/null 2>&1
compare -metric RMSE /tmp/ref.png /tmp/mlim.png /dev/null 2>&1
```

## Dependencies
Requires task 377
