# Task 268: RMSE Re-Measure After Tasks 265–267

## Description
After the three visual fixes in tasks 265–267 complete (waveform background gradient,
control strip gradient, waveform dB scale removal), measure updated RMSE to quantify the
improvement and identify remaining contributors.

**Steps:**
1. Build latest code.
2. Launch standalone on Xvfb :99 (1920×1080).
3. Crop plugin editor area (exclude standalone chrome: title bar + audio-muted banner).
4. Resize M-LIM crop to 900×500.
5. Crop reference `prol2-main-ui.jpg` to plugin area (`-crop 1712x1073+97+32`, resize to 900×500).
6. Run `compare -metric RMSE` on full image.
7. Also measure sub-regions:
   - Control strip only: 900×130 at y=370
   - Waveform area: 900×370 at y=0
   - Left level meters: 75×370 at x=0
   - Right panel (GR + Loudness): 200×370 at x=700
8. Document all RMSE values, list top remaining visual differences.
9. Save results to `screenshots/task-268-rmse-results.txt`.
10. Save screenshot crops as:
    - `screenshots/task-268-mlim-cropped.png`
    - `screenshots/task-268-ref-cropped.png`
    - `screenshots/task-268-diff-full.png`

Baseline: task 264 full-image RMSE = 0.2539 (25.4%). Target remains 0.15 (15%).

## Produces
None

## Consumes
None

## Relevant Files
Read: `Scripts/ui-test-helper.sh` — helper functions (Xvfb, screenshot, compare)
Read: `/reference-docs/reference-screenshots/prol2-main-ui.jpg` — reference screenshot
Read: `screenshots/task-264-rmse-results.txt` — baseline to compare against
Create: `screenshots/task-268-rmse-results.txt` — new measurement results

## Acceptance Criteria
- [ ] Run: `cat screenshots/task-268-rmse-results.txt` → Expected: file exists containing full-image and sub-region RMSE values
- [ ] Run: `cd /workspace/M-LIM && cmake --build build -j$(nproc) 2>&1 | tail -3` → Expected: build succeeds with no errors
- [ ] Run: `cd /workspace/M-LIM/build && ctest --output-on-failure` → Expected: all tests pass

## Tests
None

## Technical Details
Use the same methodology as tasks 256 and 264:

```bash
source Scripts/ui-test-helper.sh
start_app
screenshot /tmp/mlim-raw.png
WID=$(xdotool search --name "M-LIM" | head -1)
GEOM=$(xdotool getwindowgeometry "$WID")
# parse X, Y, W, H from GEOM
convert /tmp/mlim-raw.png -crop "${W}x$((H-64))+${X}+$((Y+64))" +repage /tmp/mlim-crop-raw.png
convert /tmp/mlim-crop-raw.png -resize 900x500! /tmp/mlim-crop.png
convert /reference-docs/reference-screenshots/prol2-main-ui.jpg \
  -crop 1712x1073+97+32 +repage -resize 900x500! /tmp/ref-crop.png
compare -metric RMSE /tmp/mlim-crop.png /tmp/ref-crop.png /tmp/diff.png 2>&1
# sub-regions
convert /tmp/mlim-crop.png -crop 900x130+0+370 +repage /tmp/mlim-strip.png
convert /tmp/ref-crop.png  -crop 900x130+0+370 +repage /tmp/ref-strip.png
compare -metric RMSE /tmp/mlim-strip.png /tmp/ref-strip.png /tmp/diff-strip.png 2>&1
convert /tmp/mlim-crop.png -crop 900x370+0+0 +repage /tmp/mlim-wave.png
convert /tmp/ref-crop.png  -crop 900x370+0+0 +repage /tmp/ref-wave.png
compare -metric RMSE /tmp/mlim-wave.png /tmp/ref-wave.png /tmp/diff-wave.png 2>&1
convert /tmp/mlim-crop.png -crop 75x370+0+0 +repage /tmp/mlim-left.png
convert /tmp/ref-crop.png  -crop 75x370+0+0 +repage /tmp/ref-left.png
compare -metric RMSE /tmp/mlim-left.png /tmp/ref-left.png /tmp/diff-left.png 2>&1
convert /tmp/mlim-crop.png -crop 200x370+700+0 +repage /tmp/mlim-right.png
convert /tmp/ref-crop.png  -crop 200x370+700+0 +repage /tmp/ref-right.png
compare -metric RMSE /tmp/mlim-right.png /tmp/ref-right.png /tmp/diff-right.png 2>&1
stop_app
```

Save PNG crops and diff images using the `screenshots/task-268-*` naming convention.
After measuring, list top 3–5 visual differences and note which (if any) are close
enough to consider the RMSE target met.

## Dependencies
Requires tasks 265, 266, 267
