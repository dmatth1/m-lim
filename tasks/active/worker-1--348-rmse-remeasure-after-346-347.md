# Task 348: RMSE Remeasure After Tasks 346 and 347

## Description

After tasks 346 (darken GR meter background) and 347 (LevelMeter segment separators) are
applied, remeasure the full-image RMSE and sub-region RMSE to confirm improvement or detect
regression.

Baseline (task-343 best): 22.05% full image, 20.20% waveform, 23.33% left meters,
29.56% right panel, 22.39% control strip.

Expected improvement from task 346: GR meter strip was at ~(99–80, 99–96, 112–143) vs
reference ~(27–38, 22–32, 26–38). Darkening to barTrackBackground should reduce per-pixel
error by ~70 units in that 12px-wide strip.

Task 347 impact: Subtle; segment separators add texture to output meter background at
rest, closer to reference LED style.

## Produces
None

## Consumes
None

## Relevant Files
Read: `Scripts/ui-test-helper.sh` — screenshot methodology
Read: `screenshots/task-343-rmse-results.txt` — baseline reference numbers

## Acceptance Criteria
- [ ] Run: build succeeds: `cmake --build M-LIM/build -j$(nproc) 2>&1 | grep -c "error:"` → Expected: `0`
- [ ] Run: full-image RMSE: `source Scripts/ui-test-helper.sh && start_app 97 && sleep 4 && screenshot "task-348-raw.png" && stop_app` then compare → Expected: RMSE ≤ 22.05% (task-343 baseline)
- [ ] Run: no sub-region regresses more than 0.5% from task-343 values

## Tests
None

## Technical Details

Use the exact same methodology as task-343/345:
1. Build latest code: `cmake --build M-LIM/build -j$(nproc)`
2. Launch on Xvfb :97 (1920x1080)
3. Screenshot with `scrot`
4. Crop: `convert screenshot.png -crop 908x500+509+325 +repage -geometry 900x500\! mlim-crop.png`
5. Reference: `convert /reference-docs/reference-screenshots/prol2-main-ui.jpg -geometry 900x500\! ref-crop.png` (note: use `-geometry 900x500\!` not `-resize`)
6. Compare: `compare -metric RMSE mlim-crop.png ref-crop.png diff.png`
7. Sub-region measurements:
   - Waveform (600x400+150+50): `compare -metric RMSE <(convert mlim-crop.png -crop 600x400+150+50 +repage png:-) <(convert ref-crop.png -crop 600x400+150+50 +repage png:-) /dev/null`
   - Left meters (30x378+0+30): same pattern
   - Right panel (100x400+800+50): same pattern
   - Control strip (900x60+0+440): same pattern

Save results to `screenshots/task-348-rmse-results.txt`.

Key check: GR meter pixel color at x=695, y=200 (in 900x500 crop) should now be dark
(~20–40 per channel) not the bright waveform gradient (~80–130 per channel).

## Dependencies
Requires tasks 346 and 347
