# Task 354: RMSE Remeasure After Tasks 349, 350, 351, and 353

## Description

After tasks 349 (TopBar height increase 24→40px), 350 (active: dim ceiling line artifact),
351 (TopBar purple tint — already done), and 353 (dim waveformCeilingLine + shift ceiling
default to -0.5f), remeasure full-image and sub-region RMSE to confirm improvement and detect
regressions.

**Correct baseline (task-352, using proper reference crop):**
- Full image: ~22.20%
- Waveform: ~20.58%
- Left meters: ~23.33%
- Right panel: ~29.56%
- Control strip: ~22.42%

NOTE: Task-348 values (22.98% full image, etc.) are INVALID — they used the wrong reference
crop (squished full image instead of crop 1712x1073+97+32). Use task-352 values above.

Expected improvements from 349+350+351+353:
- Task 349 (TopBar height): ~0.3% full-image improvement
- Task 353 (ceiling line dim + move): ~0.1–0.2% improvement from reduced bright-red top rows
- Task 351 (purple tint): minimal RMSE impact

Combined target: full-image RMSE ≤ 21.80%.

Sub-region targets: none should regress more than +0.5% from task-352 values.

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
Read: `screenshots/task-352-rmse-results.txt` — corrected baseline (use this, NOT task-348)

## Acceptance Criteria
- [ ] Run: `cmake --build build -j$(nproc) 2>&1 | grep -c "error:"` → Expected: `0`
- [ ] Run: full-image RMSE (using correct reference crop) → Expected: ≤ 21.80% (improvement from 22.20%)
- [ ] Run: waveform sub-region (600x400+150+50) RMSE → Expected: ≤ 21.08% (≤ 0.5% regression from 20.58%)
- [ ] Run: left meters (30x378+0+30) RMSE → Expected: ≤ 23.83% (≤ 0.5% regression from 23.33%)
- [ ] Run: right panel (100x400+800+50) RMSE → Expected: ≤ 30.06% (≤ 0.5% regression from 29.56%)
- [ ] Run: control strip (900x60+0+440) RMSE → Expected: ≤ 22.92% (≤ 0.5% regression from 22.42%)
- [ ] Run: top-band y=20-35 RMSE (900x16+0+20) → Expected: improvement from task-352 baseline

## Tests
None

## Technical Details

**IMPORTANT — Use CORRECT reference methodology (NOT the squish from task-348):**

1. Build: `cmake --build build -j$(nproc)`
2. Launch: `source Scripts/ui-test-helper.sh && start_app 97 && sleep 4 && screenshot "task-354-raw.png" && stop_app`
3. Crop M-LIM: `convert screenshots/task-354-raw.png -crop 908x500+509+325 +repage -resize 900x500! screenshots/task-354-mlim-crop.png`
4. **CORRECT reference** (crop first, then resize — matches task-343/task-352 methodology):
   ```bash
   convert /reference-docs/reference-screenshots/prol2-main-ui.jpg \
       -crop 1712x1073+97+32 +repage -resize 900x500! screenshots/task-354-ref-crop.png
   ```
   (Do NOT use `-geometry 900x500!` on the full image — that includes macOS chrome)
5. Full: `compare -metric RMSE screenshots/task-354-mlim-crop.png screenshots/task-354-ref-crop.png screenshots/task-354-diff.png`
6. Sub-regions: same crop coordinates as task-352
7. Top-band: `compare -metric RMSE <(convert screenshots/task-354-mlim-crop.png -crop 900x16+0+20 +repage png:-) <(convert screenshots/task-354-ref-crop.png -crop 900x16+0+20 +repage png:-) /dev/null`

Save all results to `screenshots/task-354-rmse-results.txt`.

Note: This task documents the structural reason full-image RMSE > sub-region average:
- y=0-19 (900x20 px): JUCE dark top (~37) vs macOS light bar (~200) = unavoidable
  ~59% RMSE for 20 rows, contributing permanently ~0.0165 to full-image MSE (≈ 12.8% floor).
  This floor means full-image RMSE can never go below ~13% with current methodology.
- Sub-regions at 16-23% are the authoritative parity measure.

## Dependencies
Requires tasks 349, 350, and 353
