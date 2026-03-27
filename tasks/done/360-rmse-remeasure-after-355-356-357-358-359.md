# Task 360: RMSE Remeasure After Tasks 355–359

## Description

After tasks 355 (widen/move output meter), 356 (level meter idle gradient),
357 (algorithm selector compact labels), 358 (larger knob face), and 359 (control strip
brightness), remeasure full-image and sub-region RMSE to confirm improvements and detect
regressions.

**Correct baseline (task-354, using CORRECT reference crop methodology):**
- Full image: 22.08%
- Waveform (600x400+150+50): 20.55%
- Left meters (30x378+0+30): 23.50%
- Right panel (100x400+800+50): 29.59%
- Control strip (900x60+0+440): 22.42%

Expected improvements:
- Task 355 (OutputMeter rightmost + wider): right panel RMSE ~3–6% improvement
- Task 356 (idle gradient): right panel slight improvement, full image slight improvement
- Task 357 (compact labels): control strip slight improvement
- Task 358 (larger knobs): control strip slight improvement
- Task 359 (brighter control strip): control strip ~1–2% improvement

Combined target: full-image RMSE ≤ 21.00%.

## Produces
None

## Consumes
None

## Relevant Files
Read: `Scripts/ui-test-helper.sh` — screenshot methodology
Read: `screenshots/task-354-rmse-results.txt` — baseline (or use the known values above)

## Acceptance Criteria
- [ ] Run: `cmake --build build -j$(nproc) 2>&1 | grep -c "error:"` → Expected: `0`
- [ ] Run: full image RMSE → Expected: ≤ 21.00%
- [ ] Run: right panel (100x400+800+50) RMSE → Expected: ≤ 27.00% (improvement from 29.59%)
- [ ] Run: control strip (900x60+0+440) RMSE → Expected: ≤ 22.00% (improvement from 22.42%)
- [ ] Run: waveform (600x400+150+50) RMSE → Expected: ≤ 21.05% (≤ 0.5% regression from 20.55%)
- [ ] Run: left meters (30x378+0+30) RMSE → Expected: ≤ 24.00% (≤ 0.5% regression from 23.50%)

## Tests
None

## Technical Details

**IMPORTANT — CORRECT measurement methodology (from task-354):**

1. Build: `cmake --build build -j$(nproc)`
2. Launch: `source Scripts/ui-test-helper.sh && start_app 97 && sleep 4 && screenshot "task-360-raw.png" && stop_app`
3. Crop M-LIM: `convert screenshots/task-360-raw.png -crop 908x500+509+325 +repage -resize 900x500! screenshots/task-360-mlim.png`
4. CORRECT reference:
   ```bash
   convert /reference-docs/reference-screenshots/prol2-main-ui.jpg \
       -crop 1712x1073+97+32 +repage -resize 900x500! screenshots/task-360-ref.png
   ```
5. Full: `compare -metric RMSE screenshots/task-360-mlim.png screenshots/task-360-ref.png screenshots/task-360-diff.png 2>&1`
6. Sub-regions:
   ```bash
   # Waveform
   compare -metric RMSE \
     <(convert screenshots/task-360-mlim.png -crop 600x400+150+50 +repage png:-) \
     <(convert screenshots/task-360-ref.png  -crop 600x400+150+50 +repage png:-) /dev/null
   # Left meters
   compare -metric RMSE \
     <(convert screenshots/task-360-mlim.png -crop 30x378+0+30 +repage png:-) \
     <(convert screenshots/task-360-ref.png  -crop 30x378+0+30 +repage png:-) /dev/null
   # Right panel
   compare -metric RMSE \
     <(convert screenshots/task-360-mlim.png -crop 100x400+800+50 +repage png:-) \
     <(convert screenshots/task-360-ref.png  -crop 100x400+800+50 +repage png:-) /dev/null
   # Control strip
   compare -metric RMSE \
     <(convert screenshots/task-360-mlim.png -crop 900x60+0+440 +repage png:-) \
     <(convert screenshots/task-360-ref.png  -crop 900x60+0+440 +repage png:-) /dev/null
   ```

Save all results to `screenshots/task-360-rmse-results.txt`.

Note: Task 355 moves the OutputMeter to the rightmost position (x=800–900). If task 355
is completed, the right panel crop (100x400+800+50) will show OutputMeter content instead
of LoudnessPanel. The expected RMSE there depends on whether tasks 355+356 are both done.

## Dependencies
Requires tasks 355, 356, 357, 358, 359
