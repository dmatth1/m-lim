# Task 520: RMSE Remeasure After Wave-23 Visual Parity Tasks

## Description
After completing the wave-23 visual parity tasks (505 metersafe-lighten, 507 loudness-histogram-idle-fill, 508 level-meter-idle-gradient, 509 output-meter-idle-fill, 519 waveform-db-label-opacity), capture a fresh RMSE measurement using the standard methodology and record the new baseline.

Build the standalone, launch headlessly, capture screenshot, crop, resize, and compare against the reference.

## Produces
None

## Consumes
None

## Relevant Files
Read: `Scripts/ui-test-helper.sh` — helper functions for screenshot capture
Read: `screenshots/` — previous baseline results for comparison

## Acceptance Criteria
- [ ] Run: `convert /reference-docs/reference-screenshots/prol2-main-ui.jpg -crop 1712x1073+97+32 +repage -resize 900x500! /tmp/ref.png && identify /tmp/ref.png` → Expected: `900x500`
- [ ] Run: `cmake --build build --target MLIM_Standalone_Standalone -j$(nproc) 2>&1 | tail -3` → Expected: build succeeds
- [ ] Run: Build standalone, launch on Xvfb :99, scrot 1920x1080, crop 908x500+509+325, resize 900x500!, compare against ref → Expected: Full RMSE recorded in `screenshots/wave23-rmse-results.txt`
- [ ] Run: `cat screenshots/wave23-rmse-results.txt` → Expected: Full RMSE value present; improvement over previous baseline noted

## Tests
None

## Technical Details
RMSE methodology (from harness instructions):
- Reference crop: `prol2-main-ui.jpg -crop 1712x1073+97+32 +repage -resize 900x500!`
- M-LIM: scrot 1920x1080, crop `908x500+509+325`, resize `900x500!`
- Subregions: Wave (640x500+0+0), Left (80x500+640+0), Right (180x500+720+0), Control (900x90+0+410)
- Current baseline (task-354): Full=22.08%, Wave=20.55%, Left=23.50%, Right=29.59%, Control=22.42%

Save results to `screenshots/wave23-rmse-results.txt`.

## Dependencies
Requires tasks 505, 507, 508, 509, 519 (all wave-23 visual parity tasks)
