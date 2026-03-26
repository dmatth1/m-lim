# Task 275: RMSE Re-Measure After Tasks 270–274

## Description
After the visual parity improvements from tasks 270–274 are complete
(dB scale overlay, grid line color, algorithm selector, ADVANCED strip, control strip height),
measure the updated RMSE to quantify progress and identify remaining top contributors.

**Steps:**
1. Build latest code.
2. Launch standalone on Xvfb :99 (1920×1080).
3. Crop plugin editor area (exclude standalone chrome: 64 px title bar + audio-muted banner).
4. Resize M-LIM crop to 900×500.
5. Crop reference `prol2-main-ui.jpg` to plugin area (`-crop 1712x1073+97+32`, resize to 900×500).
6. Run `compare -metric RMSE` on full image.
7. Also measure sub-regions:
   - Control strip only: 900×92 at y=408 (adjusted for new kControlStripH=92)
   - Waveform area: 900×378 at y=0 (adjusted for new waveform height)
   - Left level meters: 30×378 at x=0 (adjusted for new kInputMeterW=30)
   - Right panel (GR + Loudness): 200×378 at x=700
8. Document all RMSE values, list top remaining visual differences.
9. Save results to `screenshots/task-275-rmse-results.txt`.

Baseline: task 268 full-image RMSE = 0.2659 (26.6%). Target: 0.15 (15%).

## Produces
None

## Consumes
None

## Relevant Files
Read: `Scripts/ui-test-helper.sh` — helper functions
Read: `/reference-docs/reference-screenshots/prol2-main-ui.jpg` — reference
Read: `screenshots/task-268-rmse-results.txt` — baseline to compare against
Create: `screenshots/task-275-rmse-results.txt` — new measurement results

## Acceptance Criteria
- [ ] Run: `cat screenshots/task-275-rmse-results.txt` → Expected: file exists with full-image and sub-region RMSE values
- [ ] Run: `cmake --build build -j$(nproc) 2>&1 | tail -3` → Expected: build succeeds
- [ ] Run: `cd build && ctest --output-on-failure 2>&1 | tail -3` → Expected: all tests pass

## Tests
None

## Technical Details
Use the standard RMSE measurement methodology from tasks 256, 264, 268.

```bash
source Scripts/ui-test-helper.sh
start_app
screenshot /tmp/mlim-raw.png
WID=$(xdotool search --name "M-LIM" | head -1)
GEOM=$(xdotool getwindowgeometry "$WID")
# parse X, Y, W, H from GEOM
# Note: kControlStripH is now 92, so plugin height is still 500
convert /tmp/mlim-raw.png -crop "${W}x$((H-64))+${X}+$((Y+64))" +repage /tmp/mlim-crop-raw.png
convert /tmp/mlim-crop-raw.png -resize 900x500! /tmp/mlim-crop.png
convert /reference-docs/reference-screenshots/prol2-main-ui.jpg \
  -crop 1712x1073+97+32 +repage -resize 900x500! /tmp/ref-crop.png
compare -metric RMSE /tmp/mlim-crop.png /tmp/ref-crop.png /tmp/diff-full.png 2>&1

# Sub-regions (adjust for new kControlStripH=92)
# Control strip: y=408 (500-92=408), height=92
convert /tmp/mlim-crop.png -crop 900x92+0+408 +repage /tmp/mlim-strip.png
convert /tmp/ref-crop.png  -crop 900x92+0+408 +repage /tmp/ref-strip.png
compare -metric RMSE /tmp/mlim-strip.png /tmp/ref-strip.png /tmp/diff-strip.png 2>&1

# Waveform: y=30 (topbar), height=378 (500-30-92=378)
convert /tmp/mlim-crop.png -crop 900x378+0+30 +repage /tmp/mlim-wave.png
convert /tmp/ref-crop.png  -crop 900x378+0+30 +repage /tmp/ref-wave.png
compare -metric RMSE /tmp/mlim-wave.png /tmp/ref-wave.png /tmp/diff-wave.png 2>&1

stop_app
```

Save screenshots: task-275-mlim-cropped.png, task-275-ref-cropped.png, task-275-diff-full.png

## Dependencies
Requires tasks 270, 271, 272, 273, 274
