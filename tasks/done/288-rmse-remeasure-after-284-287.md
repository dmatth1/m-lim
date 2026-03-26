# Task 288: RMSE Re-Measure After Recent Visual Parity Tasks

## Description
After the visual parity improvements from the current batch of tasks are complete
(ControlStrip channel-linking box removal [285], rotary knob range labels [286],
GR meter LED segments [289], waveform dB labels move to left edge [290],
GAIN label below badge [291], ADVANCED button left-edge strip [292]),
measure the updated RMSE to quantify progress and identify remaining top contributors.

**Steps:**
1. Build latest code.
2. Launch standalone on Xvfb :99 (1920×1080).
3. Crop plugin editor area (exclude standalone chrome: 64 px title bar + audio-muted banner).
4. Resize M-LIM crop to 900×500.
5. Crop reference `prol2-main-ui.jpg` to plugin area (`-crop 1712x1073+97+32`, resize to 900×500).
6. Run `compare -metric RMSE` on full image.
7. Also measure sub-regions:
   - Control strip only: 900×92 at y=408
   - Waveform area: 900×378 at y=30
   - Left level meters: 30×378 at x=0
   - Right panel (GR + Loudness): 200×378 at x=700
8. Document all RMSE values, list top remaining visual differences.
9. Save results to `screenshots/task-288-rmse-results.txt`.

Baseline: task 281 full-image RMSE = 0.2617 (26.17%). Target: 0.15 (15%).

## Produces
None

## Consumes
None

## Relevant Files
Read: `/reference-docs/reference-screenshots/prol2-main-ui.jpg` — reference
Read: `screenshots/task-281-rmse-results.txt` — baseline to compare against
Create: `screenshots/task-288-rmse-results.txt` — new measurement results

## Acceptance Criteria
- [ ] Run: `cat screenshots/task-288-rmse-results.txt` → Expected: file exists with full-image and sub-region RMSE values
- [ ] Run: `cmake --build build -j$(nproc) 2>&1 | tail -3` → Expected: build succeeds

## Tests
None

## Technical Details
```bash
# Standard measurement methodology
pkill -f "M-LIM" 2>/dev/null; pkill Xvfb 2>/dev/null; sleep 1
Xvfb :99 -screen 0 1920x1080x24 &
sleep 2
DISPLAY=:99 /workspace/M-LIM/build/MLIM_artefacts/Release/Standalone/M-LIM &
sleep 5
DISPLAY=:99 scrot /tmp/mlim-raw.png
WID=$(DISPLAY=:99 xdotool search --name "M-LIM" | head -1)
GEOM=$(DISPLAY=:99 xdotool getwindowgeometry $WID)
# Parse X, Y, W, H from GEOM
# Crop: skip 64px title bar
convert /tmp/mlim-raw.png -crop ${W}x500+${X}+$((Y+64)) +repage /tmp/mlim-crop.png
convert /tmp/mlim-crop.png -resize 900x500! /tmp/mlim-900.png
convert /reference-docs/reference-screenshots/prol2-main-ui.jpg \
  -crop 1712x1073+97+32 +repage -resize 900x500! /tmp/ref-900.png
compare -metric RMSE /tmp/mlim-900.png /tmp/ref-900.png /tmp/diff-full.png 2>&1
# Sub-regions
convert /tmp/mlim-900.png -crop 900x92+0+408 +repage /tmp/mlim-strip.png
convert /tmp/ref-900.png  -crop 900x92+0+408 +repage /tmp/ref-strip.png
compare -metric RMSE /tmp/mlim-strip.png /tmp/ref-strip.png /dev/null 2>&1
convert /tmp/mlim-900.png -crop 900x378+0+30 +repage /tmp/mlim-wave.png
convert /tmp/ref-900.png  -crop 900x378+0+30 +repage /tmp/ref-wave.png
compare -metric RMSE /tmp/mlim-wave.png /tmp/ref-wave.png /dev/null 2>&1
convert /tmp/mlim-900.png -crop 30x378+0+30 +repage /tmp/mlim-lmeter.png
convert /tmp/ref-900.png  -crop 30x378+0+30 +repage /tmp/ref-lmeter.png
compare -metric RMSE /tmp/mlim-lmeter.png /tmp/ref-lmeter.png /dev/null 2>&1
convert /tmp/mlim-900.png -crop 200x378+700+30 +repage /tmp/mlim-right.png
convert /tmp/ref-900.png  -crop 200x378+700+30 +repage /tmp/ref-right.png
compare -metric RMSE /tmp/mlim-right.png /tmp/ref-right.png /dev/null 2>&1
```

Save screenshots: task-288-mlim-cropped.png, task-288-ref-cropped.png, task-288-diff-full.png

## Dependencies
Requires tasks 285, 286, 289, 290, 291, 292
