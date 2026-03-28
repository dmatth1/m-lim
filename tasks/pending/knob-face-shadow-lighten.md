# Task: Lighten Knob Face Shadow to Match Reference Silvery Appearance

## Description
Pixel sampling of the current LOOKAHEAD knob face shows the center at approximately
RGB(128, 128, 148) and the shadow (bottom-right) at RGB(79, 79, 92). The reference
Pro-L 2 shows knob faces significantly brighter — center ~RGB(188, 188, 198) and
edge ~RGB(116, 121, 139).

The discrepancy is traced to `knobFaceShadow = 0xff484860` (72, 72, 96), which is
too dark. The radial gradient produces a center average ~(128, 148) instead of the
reference target ~(188, 198).

**Fix**: Change `knobFaceShadow` from `0xff484860` to `0xff8F8F9B` (143, 143, 155).
This makes the gradient shadow end closer to the reference measured edge (~116,121,139),
shifting the knob face center toward the target (188, 188, 198).

The expected center after fix (using JUCE radial gradient interpolation at t≈0.46):
  0.54 × (221, 221, 232) + 0.46 × (143, 143, 155) = (119+66, 119+66, 125+71) ≈ (185, 185, 196)

That is within ~3 units of the reference target (188, 188, 198). The 3D highlight effect
is preserved (highlight is still ~60% brighter than shadow).

## Produces
None

## Consumes
None

## Relevant Files
Modify: `src/ui/Colours.h` — change `knobFaceShadow` constant (line ~44)

## Acceptance Criteria
- [ ] Run: `grep 'knobFaceShadow' src/ui/Colours.h` → Expected: shows `0xff8F8F9B`
- [ ] Run: `cmake --build build --target MLIM_Standalone -j$(nproc)` → Expected: build succeeds, exit 0
- [ ] Run: launch app, capture screenshot, sample pixel at LOOKAHEAD knob center → Expected: knob center reads approximately (170–200, 170–200, 180–210) vs previous (~128, 128, 148)
- [ ] Run: RMSE measurement → Expected: Control strip RMSE ≤ 19.37% (no regression)

## Tests
None

## Technical Details
Single constant change in `Colours.h`:

```cpp
// Before:
const juce::Colour knobFaceShadow   { 0xff484860 };  // bluer shadow — blue-gray tint across face (task-453)
// After:
const juce::Colour knobFaceShadow   { 0xff8F8F9B };  // lightened shadow to match reference ~(143,143,155) — knob centre ~(185,185,196) (task-NNN)
```

Knob centre pixel sampling methodology:
- Launch app headlessly on Xvfb :99 (1920x1080)
- Scrot full screenshot, crop 908x500+509+325, resize 900x500
- LOOKAHEAD knob centre ≈ (213, 450) in the 900x500 image
- Sample: `convert screenshot.png -crop 1x1+213+450 +repage txt:-`

Expected RMSE improvement: ~1.0–1.5pp for Control zone (currently 19.37%),
~0.3–0.5pp for Full RMSE (currently 19.07%).

## Dependencies
None
