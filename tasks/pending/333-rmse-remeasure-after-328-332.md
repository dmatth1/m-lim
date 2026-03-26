# Task 333: RMSE Remeasure After Visual Tasks 328–332

## Description
Remeasure full-image and sub-region RMSE after tasks 328–332 are merged, to track progress
toward the 15% target.

Tasks covered:
- 328: Revert output meter width (kOutputMeterW 80→58)
- 329: Revert control strip knob size (kKnobRowH 72→56, kControlStripH 108→92)
- 330: Darken right panel background (loudnessPanelBackground #2B2729→#1E1C21)
- 331: Waveform gradient colors brighter bottom (displayGradientBottom #506090→#6878A8)
- 332: Loudness panel histogram taller (kRowH 22→18, kLargeReadoutH 62→48)

## Produces
None

## Consumes
artifact:M-LIM/build/MLIM_artefacts/Release/Standalone/M-LIM

## Relevant Files
Read: `Scripts/ui-test-helper.sh` — start_app, screenshot, compare_to_reference helpers
Read: `screenshots/task-324-rmse-results.txt` — baseline for comparison

## Acceptance Criteria
- [ ] Run: `source Scripts/ui-test-helper.sh && start_app && screenshot "task-333-raw.png" && stop_app` → Expected: screenshot saved to screenshots/
- [ ] Run: RMSE comparison → Expected: full-image RMSE < 0.220 (target: improvement from 23.14%)
- [ ] Save results to `screenshots/task-333-rmse-results.txt`

## Tests
None

## Technical Details
Methodology (same as task 324):
```bash
source Scripts/ui-test-helper.sh
start_app

# Capture and crop plugin area (excludes 64px standalone chrome)
screenshot "task-333-raw.png"
# Crop: 908x500+509+325 → resize 900x500
convert screenshots/task-333-raw.png -crop 908x500+509+325 -resize 900x500 screenshots/task-333-mlim-cropped.png

# Reference crop (same as 317/321/324)
convert /reference-docs/reference-screenshots/prol2-main-ui.jpg \
  -crop 1712x1073+97+32 -resize 900x500 screenshots/task-333-ref-cropped.png

# Full image RMSE
compare -metric RMSE screenshots/task-333-ref-cropped.png screenshots/task-333-mlim-cropped.png \
  screenshots/task-333-diff-full.png 2>&1

# Sub-region measurements (same regions as tasks 317/321/324)
# Left meters: 30x378+0+30
# Waveform: 600x400+150+50  — note: with kControlStripH=92 after task 329, the waveform
#   extends to y=384, so this crop (y=50 to y=450) is still valid
# Right panel: 100x400+800+50
# Control strip: 900x60+0+440

stop_app
```

Note: After task 329 reverts kControlStripH to 92, the standalone window crop parameters
may still be 908x500+509+325 (the plugin content starts at y=325 from the top of the
1920x1080 screen, which includes the JUCE standalone chrome). Verify the plugin window
position has not changed before capturing.

Save full results (baseline 324, current) to `screenshots/task-333-rmse-results.txt`.
Optimize screenshots: `find screenshots/ -name "*.png" -exec optipng -o2 -fix -quiet {} \;`

## Dependencies
Requires tasks 328, 329, 330 (and optionally 331, 332)
