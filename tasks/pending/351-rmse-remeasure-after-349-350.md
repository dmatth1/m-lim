# Task 351: RMSE Remeasure After Tasks 349 and 350

## Description

After tasks 349 (TopBar height increase 24→40px) and 350 (ceiling line dim), remeasure
full-image and sub-region RMSE to confirm improvement and detect regressions.

Baseline (task-348): 22.98% full image, 19.63% waveform, 20.05% left meters,
21.97% right panel, 15.92% control strip.

Expected from task 349: ~0.32% full-image improvement (22.98% → ~22.66%)
Expected from task 350: minor additional improvement (< 0.1%)

Combined target: full-image RMSE ≤ 22.50%.

Sub-region targets: none should regress more than +0.5% from task-348 values.

Key explanation for workers: the full-image RMSE is ~3% higher than sub-region average
because the top 20 rows (crop y=0-19) show the JUCE dark TopBar (~37,37,37) vs the
macOS light title bar in the reference (~200,200,200). This platform difference creates
~59% RMSE for just those 20 rows and is irreducible without platform-specific hacks.
The sub-region measurements are a more meaningful parity indicator.

## Produces
None

## Consumes
None

## Relevant Files
Read: `Scripts/ui-test-helper.sh` — screenshot methodology
Read: `screenshots/task-348-rmse-results.txt` — baseline

## Acceptance Criteria
- [ ] Run: `cmake --build build -j$(nproc) 2>&1 | grep -c "error:"` → Expected: `0`
- [ ] Run: full-image RMSE after crop+compare → Expected: ≤ 22.50% (improvement from 22.98%)
- [ ] Run: waveform sub-region (600x400+150+50) RMSE → Expected: ≤ 20.46% (≤ 0.5% regression from 19.63%)
- [ ] Run: left meters (30x378+0+30) RMSE → Expected: ≤ 20.55% (≤ 0.5% regression from 20.05%)
- [ ] Run: right panel (100x400+800+50) RMSE → Expected: ≤ 22.47% (≤ 0.5% regression from 21.97%)
- [ ] Run: control strip (900x60+0+440) RMSE → Expected: ≤ 16.42% (≤ 0.5% regression from 15.92%)
- [ ] Run: top-band y=20-35 RMSE (900x16+0+20) → Expected: ≤ 0.15 (improvement from 0.219)

## Tests
None

## Technical Details

Use the exact same methodology as task-348:
1. Build: `cmake --build build -j$(nproc)`
2. Launch: `source Scripts/ui-test-helper.sh && start_app 97 && sleep 4 && screenshot "task-351-raw.png" && stop_app`
3. Crop: `convert screenshots/task-351-raw.png -crop 908x500+509+325 +repage -geometry 900x500\! screenshots/task-351-mlim-crop.png`
4. Reference: `convert /reference-docs/reference-screenshots/prol2-main-ui.jpg -geometry 900x500\! screenshots/task-351-ref-crop.png`
5. Full: `compare -metric RMSE screenshots/task-351-mlim-crop.png screenshots/task-351-ref-crop.png screenshots/task-351-diff.png`
6. Sub-regions: same crop coordinates as task-348
7. New: top-band (y=20-35): `compare -metric RMSE <(convert screenshots/task-351-mlim-crop.png -crop 900x16+0+20 +repage png:-) <(convert screenshots/task-351-ref-crop.png -crop 900x16+0+20 +repage png:-) /dev/null`

Save all results to `screenshots/task-351-rmse-results.txt`.

Note: This task also documents the structural reason full-image RMSE > sub-region average:
- y=0-19 (900x20 px): JUCE dark top (~37) vs macOS light bar (~200) = unavoidable
  ~59% RMSE for 20 rows, contributing permanently ~0.0165 to full-image MSE (≈ 12.8% floor).
  This floor means full-image RMSE can never go below ~13% with current methodology.
- Sub-regions at 16-22% are the authoritative parity measure.

## Dependencies
Requires tasks 349 and 350
