# Task: Darken Control Strip Background to Match Reference

## Description
Pixel analysis of the wave-22 screenshot vs the Pro-L 2 reference shows the control strip
gradient background is visually ~18–20 units brighter than the reference.

Measurements (900x500 crop, y=411–413 = pure background, above knob circles):
- M-LIM: R=104, G=100, B=118
- Reference: R=85, G=85, B=97
- Delta: ΔR=-19, ΔG=-15, ΔB=-21

Wave-22 baseline Control RMSE = 19.37%. Wave-21 was 21.02%. Task-392 brightened the strip
by +13 units. The strip may now overshoot the optimum — worker must measure RMSE both before
and after any change to confirm it is an improvement.

Proposed target values (matching reference pixel sample):
- `controlStripTop`    0xff696578 → 0xff555561 (R=85, G=85, B=97)
- `controlStripBottom` 0xff565362 → 0xff3C3B47 (R=60, G=59, B=71)

If these full-target values worsen RMSE, try a half-step first:
- `controlStripTop`    → 0xff5F5B6C (R=95, G=91, B=108)
- `controlStripBottom` → 0xff4A4857 (R=74, G=72, B=87)

## Produces
None

## Consumes
None

## Relevant Files
Modify: `src/ui/Colours.h` — change controlStripTop and controlStripBottom constants
Read:   `src/ui/ControlStrip.cpp` — confirm gradient uses controlStripTop/Bottom
Read:   `screenshots/task-411-wave22-crop.png` — wave-22 reference crop for RMSE baseline

## Acceptance Criteria
- [ ] Run RMSE methodology → Expected: Control zone RMSE ≤ 19.37% (wave-22 baseline)
- [ ] Run RMSE methodology → Expected: Full RMSE ≤ 19.11% (wave-22 full baseline)
- [ ] Visual inspection: control strip noticeably darker, closer to Pro-L 2 reference

## Tests
None

## Technical Details
RMSE methodology (all from project root `/workspace/M-LIM`):
```bash
# Capture screenshot
Xvfb :99 -screen 0 1920x1080x24 &; sleep 1
DISPLAY=:99 build/MLIM_artefacts/Release/Standalone/M-LIM &; sleep 5
DISPLAY=:99 scrot -o /tmp/task-raw.png

# Crop to standard comparison region
convert /tmp/task-raw.png -crop 908x500+509+325 +repage -resize 900x500! /tmp/task-mlim.png
convert /reference-docs/reference-screenshots/prol2-main-ui.jpg \
    -crop 1712x1073+97+32 +repage -resize 900x500! /tmp/task-ref.png

# Full RMSE
compare -metric RMSE /tmp/task-ref.png /tmp/task-mlim.png /dev/null 2>&1

# Control zone RMSE (900x90+0+410)
convert /tmp/task-mlim.png -crop 900x90+0+410 +repage /tmp/task-ctrl.png
convert /tmp/task-ref.png  -crop 900x90+0+410 +repage /tmp/ref-ctrl.png
compare -metric RMSE /tmp/task-ref.png /tmp/task-ctrl.png /dev/null 2>&1
```

The control strip gradient is drawn in `ControlStrip.cpp` using `MLIMColours::controlStripTop`
and `MLIMColours::controlStripBottom` as the vertical gradient endpoints.

## Dependencies
None
