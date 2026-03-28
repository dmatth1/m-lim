# Task: RMSE Remeasure After Wave 23 Visual Parity Tasks

## Description
After completing the wave-23 visual parity tasks (metersafe-lighten, level-meter-idle-gradient, loudness-histogram-idle-fill, waveform-db-label-opacity, output-meter-idle-fill), capture a fresh RMSE measurement using the standard methodology and record the new baseline.

Build the standalone, launch headlessly, capture screenshot, crop, resize, and compare against the reference.

## Produces
None

## Consumes
None

## Relevant Files
Read: `Scripts/ui-test-helper.sh` — helper functions for screenshot capture
Read: `screenshots/task-416-rmse-results.txt` — previous baseline for comparison

## Acceptance Criteria
- [ ] Run: `cmake --build build --target MLIM_Standalone -j$(nproc)` → Expected: clean build
- [ ] Run: `source Scripts/ui-test-helper.sh && start_app && screenshot "wave23-rmse.png" && stop_app` → Expected: screenshot captured
- [ ] Run: RMSE comparison using correct methodology → Expected: results saved to `screenshots/wave23-rmse-results.txt`
- [ ] Run: `cat screenshots/wave23-rmse-results.txt` → Expected: Full RMSE < 18.5% (improvement from 19.07%)

## Tests
None

## Technical Details
RMSE methodology (from harness instructions):
- Reference crop: `prol2-main-ui.jpg -crop 1712x1073+97+32 +repage -resize 900x500!`
- M-LIM: scrot 1920x1080, crop `908x500+509+325`, resize `900x500!`
- Zones: Full, Wave (600x400 at 150,50), Left (30x378 at 0,30), Right (100x400 at 800,50), Control (900x60 at 0,440)
- Baseline (task-416): Full=19.07%, Wave≈16.51%, Left=26.22%, Right=23.32%, Control≈19.37%

Save results to `screenshots/wave23-rmse-results.txt` with format matching `task-411-rmse-results.txt`.

## Dependencies
Requires all wave-23 visual parity tasks to be completed first
