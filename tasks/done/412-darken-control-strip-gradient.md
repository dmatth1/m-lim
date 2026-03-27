# Task 412: Darken Control Strip Gradient to Match Reference

## Description
Pixel analysis of the wave-22 screenshot vs the Pro-L 2 reference shows the control strip
gradient background is visually ~18–27 units brighter than the reference.

Measurements (900x500 crop, y=411–413 = pure background, above knob circles):
- M-LIM: R=104, G=100, B=118
- Reference: R=85, G=85, B=97
- Delta: ΔR=-19, ΔG=-15, ΔB=-21

Top label-row gap (y=2–17 of control zone): M-LIM avg #666374 vs Ref #4B4B58 — M-LIM +27R, +24G, +28B too bright.
Mid knob area (y=35–60): M-LIM avg #5D5B6C vs Ref #535568 — M-LIM ~+10 units brighter.
Bottom row (y=70–88): M-LIM avg #52515D vs Ref #424150 — M-LIM +16R, +16G, +13B too bright.

Task-392 previously brightened the control strip by +13. The strip now overshoots the reference by ~25 units at the top label row. The strip needs to be darkened to close this gap.

Wave-22 baseline Control RMSE = 19.37%.

## Produces
None

## Consumes
None

## Relevant Files
Modify: `src/ui/Colours.h` — change `controlStripTop` and `controlStripBottom` constants
Read:   `src/ui/ControlStrip.cpp` — confirm gradient uses controlStripTop/Bottom

## Acceptance Criteria
- [ ] Run RMSE methodology → Expected: Control zone RMSE ≤ 19.37% (wave-22 baseline)
- [ ] Run RMSE methodology → Expected: Full RMSE ≤ 19.11% (wave-22 full baseline)
- [ ] Visual inspection: control strip noticeably darker, closer to Pro-L 2 reference

## Tests
None

## Technical Details
Current values in `Colours.h`:
```cpp
const juce::Colour controlStripTop    { 0xff696578 };  // +13 brightening from task-387 value
const juce::Colour controlStripBottom { 0xff565362 };  // +12 brightening from task-387 value
```

**Conservative target (half-step, ~-12 to -14 units):**
```cpp
const juce::Colour controlStripTop    { 0xff5A5769 };  // R-15/G-14/B-15 from current
const juce::Colour controlStripBottom { 0xff4A4856 };  // R-12/G-11/B-12 from current
```

**Full target (match reference pixel sample):**
```cpp
const juce::Colour controlStripTop    { 0xff555561 };  // R=85, G=85, B=97
const juce::Colour controlStripBottom { 0xff3C3B47 };  // R=60, G=59, B=71
```

Start with the conservative target, measure RMSE. If Control RMSE improves, try the full
target. If full target worsens RMSE, keep the conservative half-step.

RMSE methodology (all from `/workspace/M-LIM`):
```bash
Xvfb :99 -screen 0 1920x1080x24 & sleep 1
DISPLAY=:99 build/MLIM_artefacts/Release/Standalone/M-LIM & sleep 5
DISPLAY=:99 scrot -o /tmp/task-raw.png

convert /tmp/task-raw.png -crop 908x500+509+325 +repage -resize 900x500! /tmp/task-mlim.png
convert /reference-docs/reference-screenshots/prol2-main-ui.jpg \
    -crop 1712x1073+97+32 +repage -resize 900x500! /tmp/task-ref.png

# Full RMSE
compare -metric RMSE /tmp/task-ref.png /tmp/task-mlim.png /dev/null 2>&1

# Control zone RMSE (900x90+0+410)
convert /tmp/task-mlim.png -crop 900x90+0+410 +repage /tmp/task-ctrl.png
convert /tmp/task-ref.png  -crop 900x90+0+410 +repage /tmp/ref-ctrl.png
compare -metric RMSE /tmp/ref-ctrl.png /tmp/task-ctrl.png /dev/null 2>&1
```

## Dependencies
None
