# Task 349: RMSE Re-Baseline With Correct Reference Crop

## Description

Task-348's RMSE measurements are INVALID and must be discarded. The task-348 worker used
`prol2-main-ui.jpg -resize 900x500!` (full 1900×1184 image squished) instead of the correct
crop that all prior measurements used. This included macOS window chrome and desktop background
in the reference, making the comparison meaningless.

**Measured proof (auditor, 2026-03-27):**
- Using correct reference crop (1712x1073+97+32 → 900x500): full-image RMSE = **22.20%**
- Using task-348 full-resize reference: full-image RMSE = **22.98%** (inflated by ~0.78%)
- Sub-region results from task-348 are also unreliable for the same reason

The correct reference command (matching task-343 methodology) is:
```
convert /reference-docs/reference-screenshots/prol2-main-ui.jpg \
    -crop 1712x1073+97+32 +repage -resize 900x500! /tmp/ref-crop.png
```

The current state (post tasks 346+347, using correct reference):
- Full image: ~22.20%  (essentially unchanged from task-343's 22.05%)
- Waveform: ~20.58%
- Left meters: ~23.33%
- Right panel: ~29.56%
- Control strip: ~22.42%

Tasks 346 and 347 did not measurably change the RMSE relative to the correct reference.

## Produces
None

## Consumes
None

## Relevant Files
Read: `scripts/ui-test-helper.sh` — contains screenshot + compare helpers
Read: `screenshots/task-343-rmse-results.txt` — gold-standard reference for correct methodology
Modify: `screenshots/task-349-rmse-results.txt` — write fresh measurement results here

## Acceptance Criteria
- [ ] Run: `convert /reference-docs/reference-screenshots/prol2-main-ui.jpg -crop 1712x1073+97+32 +repage -resize 900x500! /tmp/ref349.png && identify /tmp/ref349.png` → Expected: `900x500`
- [ ] Run: full-image RMSE measurement with M-LIM standalone vs `/tmp/ref349.png` → Expected: result is documented; should be in range 21–24%
- [ ] Sub-region measurements completed (waveform, left meters, right panel, control strip)
- [ ] `screenshots/task-349-rmse-results.txt` created with all measurements and notes that task-348 results were invalid

## Tests
None

## Technical Details

**Correct reference generation command:**
```bash
convert /reference-docs/reference-screenshots/prol2-main-ui.jpg \
    -crop 1712x1073+97+32 +repage -resize 900x500! /tmp/ref349.png
```

**M-LIM standalone crop (unchanged from all prior tasks):**
```bash
# 1. Launch on Xvfb :97 (1920x1080)
# 2. scrot full 1920x1080
# 3. convert raw.png -crop 908x500+509+325 +repage -resize 900x500! mlim-cropped.png
```

**RMSE measurement:**
```bash
compare -metric RMSE mlim-cropped.png /tmp/ref349.png /dev/null
# Normalized value (the parenthesised one) is the 0.0–1.0 RMSE
```

**Sub-region RMSE:**
```bash
# Waveform
compare -metric RMSE \
    <(convert mlim-cropped.png -crop 600x400+150+50 +repage png:-) \
    <(convert /tmp/ref349.png  -crop 600x400+150+50 +repage png:-) /dev/null

# Left meters
compare -metric RMSE \
    <(convert mlim-cropped.png -crop 30x378+0+30 +repage png:-) \
    <(convert /tmp/ref349.png  -crop 30x378+0+30 +repage png:-) /dev/null

# Right panel
compare -metric RMSE \
    <(convert mlim-cropped.png -crop 100x400+800+50 +repage png:-) \
    <(convert /tmp/ref349.png  -crop 100x400+800+50 +repage png:-) /dev/null

# Control strip
compare -metric RMSE \
    <(convert mlim-cropped.png -crop 900x60+0+440 +repage png:-) \
    <(convert /tmp/ref349.png  -crop 900x60+0+440 +repage png:-) /dev/null
```

## Dependencies
None
