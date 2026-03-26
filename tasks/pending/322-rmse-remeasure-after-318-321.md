# Task 322: RMSE Remeasure After Visual Tasks 318–321

## Description
Remeasure full-image and sub-region RMSE after tasks 318–321 are merged, to track progress
toward the 15% target.

Tasks covered:
- 318: Loudness histogram bar color (below-target bins → steel-blue)
- 319: Loudness histogram scale no-skip (show −15 label near −14 target)
- 320: GR meter remove numeric readout (kNumericH → 0)
- 321: Control strip taller knobs (kKnobRowH 56→70, kControlStripH 92→110)

## Produces
None

## Consumes
artifact:M-LIM/build/MLIM_artefacts/Release/Standalone/M-LIM

## Relevant Files
Read:   `Scripts/ui-test-helper.sh` — start_app, screenshot, compare_to_reference helpers
Read:   `screenshots/task-317-rmse-results.txt` — baseline for comparison

## Acceptance Criteria
- [ ] Run: `source Scripts/ui-test-helper.sh && start_app && screenshot "task-322-raw.png" && stop_app` → Expected: screenshot saved to screenshots/
- [ ] Run: RMSE comparison → Expected: full-image RMSE < 0.230 (already passing), right panel RMSE improved from 29.61%, overall RMSE closer to 20%
- [ ] Save results to `screenshots/task-322-rmse-results.txt`

## Tests
None

## Technical Details
Methodology (same as task 317):
```bash
source Scripts/ui-test-helper.sh
start_app

# Capture and crop plugin area (excludes 64px standalone chrome)
screenshot "task-322-raw.png"
# Crop: 908x500+509+325 → resize 900x500
convert screenshots/task-322-raw.png -crop 908x500+509+325 -resize 900x500 screenshots/task-322-mlim-cropped.png

# Reference crop (same as 317)
convert /reference-docs/reference-screenshots/prol2-main-ui.jpg \
  -crop 1712x1073+97+32 -resize 900x500 screenshots/task-322-ref-cropped.png

# Full image RMSE
compare -metric RMSE screenshots/task-322-ref-cropped.png screenshots/task-322-mlim-cropped.png \
  screenshots/task-322-diff-full.png 2>&1

# Sub-region measurements (same regions as task 317)
# Left meters: 30x378+0+30  Waveform: 600x400+150+50
# Right panel: 100x400+800+50  Control strip: 900x60+0+440
stop_app
```

Save full results (baseline, current, per-region) to `screenshots/task-322-rmse-results.txt`.
Optimize screenshots: `find screenshots/ -name "*.png" -exec optipng -o2 -fix -quiet {} \;`

## Dependencies
Requires tasks 318, 319, 320, 321
