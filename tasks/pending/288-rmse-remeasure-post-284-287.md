# Task 288: RMSE Re-Measure After Tasks 284–287

## Description
After tasks 284–287 are merged, re-run the RMSE comparison between M-LIM and the Pro-L 2
reference screenshot to measure improvement. Capture before/after screenshots and compute full-image
and sub-region RMSE values.

## Produces
None

## Consumes
None

## Relevant Files
Read: `Scripts/ui-test-helper.sh` — screenshot capture and RMSE comparison helpers
Read: `/reference-docs/reference-screenshots/prol2-main-ui.jpg` — reference for RMSE
Read: `screenshots/task-281-rmse-results.txt` — previous baseline (full=0.2617, 26.17%)

## Acceptance Criteria
- [ ] Run: `source Scripts/ui-test-helper.sh && start_app && screenshot screenshots/task-288-after.png && stop_app` → Expected: screenshot file created
- [ ] Run: `compare -metric RMSE screenshots/task-288-mlim-cropped.png screenshots/task-288-ref-cropped.png NULL: 2>&1` → Expected: RMSE value reported
- [ ] RMSE result saved to `screenshots/task-288-rmse-results.txt` with methodology notes

## Tests
None

## Technical Details
Follow the same methodology as task 281:
1. Build latest (Release)
2. Launch on Xvfb :99 (1920×1080)
3. Capture raw screenshot, crop plugin area 908×500 (exclude 64px standalone chrome)
4. Resize to 900×500
5. Crop reference prol2-main-ui.jpg to 1712×1073+97+32, resize to 900×500
6. Run `convert -metric RMSE` for full image and sub-regions
7. Document findings in `screenshots/task-288-rmse-results.txt`

## Dependencies
Requires tasks 284, 285, 286, 287
