# Task: RMSE Re-Measurement After Tasks 410–475

## Description
The last RMSE baseline was captured at task-409 (Full: 18.95%, Wave: 16.06%). Since then,
approximately 65 additional tasks have been completed (410–475), many of which target
specific visual zones. A new baseline measurement is needed to:
1. Confirm the cumulative improvement
2. Identify which subregions still have the highest RMSE
3. Set a fresh target for the next round of visual parity work

**Critical subregions to measure (from task-354 baseline comparisons):**
- Wave region (left 640px of 900x500 crop): was 20.55% at task-354, 16.06% at task-409
- Left meters (80px, x=640-720): was 23.50% at task-354
- Right panel (180px, x=720-900): was 29.59% at task-354 (highest RMSE subregion)
- Control strip (bottom 90px): was 22.42% at task-354
- Full image: was 22.08% at task-354, 18.95% at task-409

## Produces
None

## Consumes
None

## Relevant Files
Read: `screenshots/task-409-rmse-results.txt` — last known baseline for comparison
Read: `src/ui/Colours.h` — current colour constants (many tuned since task-409)
Read: `src/ui/WaveformDisplay.cpp` — current waveform rendering

## Acceptance Criteria
- [ ] Run: `convert /reference-docs/reference-screenshots/prol2-main-ui.jpg -crop 1712x1073+97+32 +repage -resize 900x500! /tmp/ref.png && identify /tmp/ref.png` → Expected: `900x500`
- [ ] Run: Build standalone, launch on Xvfb :99, capture scrot 1920x1080, crop 908x500+509+325, resize 900x500!, save to `/tmp/mlim-current.png`
- [ ] Run: `compare -metric RMSE /tmp/mlim-current.png /tmp/ref.png /dev/null 2>&1` → Expected: reports RMSE value — capture for comparison
- [ ] Run: Subregion comparisons (wave, left meters, right panel, control strip) as per task-354 methodology
- [ ] Save results to `screenshots/rmse-after-task-475.txt`

## Tests
None

## Technical Details
**Reference crop methodology (CORRECT — do not use full-image resize):**
```bash
convert /reference-docs/reference-screenshots/prol2-main-ui.jpg \
  -crop 1712x1073+97+32 +repage -resize 900x500! /tmp/ref.png
```

**M-LIM crop methodology:**
```bash
# After launching on Xvfb :99 and taking scrot:
convert /tmp/mlim-screenshot.png -crop 908x500+509+325 +repage -resize 900x500! /tmp/mlim-current.png
```

**Subregion commands:**
```bash
# Wave region (left 640px)
compare -metric RMSE <(convert /tmp/mlim-current.png -crop 640x500+0+0 +repage png:-) \
                     <(convert /tmp/ref.png -crop 640x500+0+0 +repage png:-) /dev/null 2>&1

# Left meters (80px)
compare -metric RMSE <(convert /tmp/mlim-current.png -crop 80x500+640+0 +repage png:-) \
                     <(convert /tmp/ref.png -crop 80x500+640+0 +repage png:-) /dev/null 2>&1

# Right panel (180px)
compare -metric RMSE <(convert /tmp/mlim-current.png -crop 180x500+720+0 +repage png:-) \
                     <(convert /tmp/ref.png -crop 180x500+720+0 +repage png:-) /dev/null 2>&1

# Control strip (bottom 90px)
compare -metric RMSE <(convert /tmp/mlim-current.png -crop 900x90+0+410 +repage png:-) \
                     <(convert /tmp/ref.png -crop 900x90+0+410 +repage png:-) /dev/null 2>&1
```

After measurement, analyze which subregion has the highest RMSE and create follow-up colour
adjustment tasks for the worst zone.

## Dependencies
None
