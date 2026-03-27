# Task: Brighten Rotary Knob Face to Match Reference Silver Appearance

## Description
Pixel analysis of the control strip shows M-LIM knobs are significantly darker than
the reference FabFilter Pro-L 2 knobs:

- Reference knob highlight area: `#DBDBE4` (R=219, G=219, B=228) — bright silver
- M-LIM knob at same position: `#7F7F88` (R=127, G=127, B=136) — medium gray

The `knobFaceHighlight = 0xffC0C0D0` (R=192) is close to the reference's R=219, but
the rendered gradient output is dimmer than expected. The `knobFaceShadow = 0xff303030`
(R=48) is very dark, pulling the overall appearance dark.

Brightening both constants will make the knob gradient span from brighter highlight to
less-dark shadow, producing a more metallic silver appearance matching the reference.

**Expected RMSE gain:** Control strip −0.4 to −0.6pp.

## Produces
None

## Consumes
None

## Relevant Files
Modify: `src/ui/Colours.h` — `knobFaceHighlight`, `knobFaceShadow` constants
Read: `src/ui/RotaryKnob.cpp` — confirms gradient usage in `paint()`

## Acceptance Criteria
- [ ] Run: build + screenshot + RMSE → Expected: Control RMSE ≤ 20.3% (down from 20.65%)
- [ ] Run: Full RMSE → Expected: ≤ 21.1% (no regression)
- [ ] Run: visual check → Expected: knobs appear brighter/more metallic silver with clear highlight

## Tests
None

## Technical Details

In `src/ui/Colours.h`, change two constants:

```cpp
// Before:
const juce::Colour knobFaceHighlight{ 0xffC0C0D0 };  // was 0xffC0C0D0
const juce::Colour knobFaceShadow   { 0xff303030 };  // neutral dark shadow

// After:
const juce::Colour knobFaceHighlight{ 0xffDDDDE8 };  // brighter blue-tinted highlight (~reference #DBDBE4)
const juce::Colour knobFaceShadow   { 0xff505060 };  // lighter shadow — less dark overall
```

**Why:** The radial gradient in `RotaryKnob.cpp::paint()` interpolates from
`knobFaceHighlight` (center-upper-left) to `knobFaceShadow` (outer-lower-right).
With shadow at #303030, the mid-tone of the gradient (~50% position) renders at
about R=120, while reference mid-tones are R=152. Raising shadow to #505060
raises mid-tones proportionally without blowing out the highlight.

The `knobFace` constant is defined in `Colours.h` but NOT used in the actual
`RotaryKnob.cpp` paint — only `knobFaceHighlight` and `knobFaceShadow` are used
in the gradient fill, so only those two need changing.

**Measurement:**
```bash
# Control strip RMSE (y=410-500, full width)
convert /tmp/mlim.png -crop 900x90+0+410 +repage /tmp/ctrl.png
convert /tmp/ref.png  -crop 900x90+0+410 +repage /tmp/ref_ctrl.png
compare -metric RMSE /tmp/ref_ctrl.png /tmp/ctrl.png /dev/null 2>&1
```

## Dependencies
None
