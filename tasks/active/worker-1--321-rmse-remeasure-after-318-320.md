# Task 321: RMSE Remeasure After Visual Tasks 318–320

## Description
Remeasure full-image and sub-region RMSE after merging tasks 318 (larger knobs),
319 (level meter smooth fill), and 320 (wider output meter).

Rebuild from latest main, launch on Xvfb, capture screenshot, compare against reference.
Report per-region RMSE and compare to task-317 baseline.

## Produces
None

## Consumes
artifact:build/MLIM_artefacts/Release/Standalone/M-LIM

## Relevant Files
Read: `screenshots/task-317-rmse-results.txt` — baseline measurements to compare against
Read: `Scripts/ui-test-helper.sh` — helper script for screenshot capture

## Acceptance Criteria
- [ ] Run: `source Scripts/ui-test-helper.sh && start_app && screenshot screenshots/task-321-mlim-cropped.png && stop_app` → Expected: screenshot captured
- [ ] Run: compare full RMSE against reference → Expected: report final RMSE value
- [ ] Run: report sub-region RMSE for waveform, right panel, left meters, control strip

## Tests
None

## Technical Details
Methodology (same as task 317):
1. Build latest main
2. Launch on Xvfb :99
3. Capture window: `scrot -a 509,261,908,564 screenshots/task-321-raw-window.png`
4. Crop plugin area (exclude 64px standalone chrome): `convert ... -crop 908x500+0+64 +repage -resize 900x500\! screenshots/task-321-mlim-cropped.png`
5. Prepare reference: `convert /reference-docs/reference-screenshots/prol2-main-ui.jpg -crop 1712x1073+97+32 +repage -resize 900x500\! screenshots/task-321-ref-cropped.png`
6. Measure: `compare -metric RMSE screenshots/task-321-mlim-cropped.png screenshots/task-321-ref-cropped.png /dev/null`
7. Sub-regions same as task 317

Save results to `screenshots/task-321-rmse-results.txt`.

Baseline (task 317):
- Full: 21.97%
- Waveform: 20.00%
- Right panel: 29.61%
- Left meters: 23.45%
- Control strip: 22.57%

Target: RMSE < 21.0% (improvement of at least 1pp from task 317 baseline).

## Dependencies
Requires tasks 318, 319, 320
