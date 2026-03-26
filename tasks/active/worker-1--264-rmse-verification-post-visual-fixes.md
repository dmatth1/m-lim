# Task 264: RMSE Verification After Visual Fixes 257-263

## Description
After tasks 257–263 complete (level meter LED segments, algorithm selector buttons, waveform
symmetric rendering, GR meter cleanup, waveform label hide, True Peak indicator, loudness panel
button layout), measure the updated RMSE to quantify the visual improvement and identify any
remaining large contributors.

**Steps:**
1. Build latest code.
2. Launch standalone on Xvfb :99 (1920×1080).
3. Use `xdotool` to get window position.
4. Crop the plugin editor area (exclude standalone chrome: title bar + audio-muted banner).
5. Resize M-LIM crop to 900×500.
6. Crop reference `prol2-main-ui.jpg` to plugin area (`-crop 1712x1073+97+32`, resize to 900×500).
7. Run `compare -metric RMSE`.
8. Also compute RMSE for the **control strip only** (crop `0,370,900,130` from both images).
9. Document findings, list top remaining visual differences, and update SPEC.md if needed.
10. Save results to `screenshots/task-264-rmse-results.txt`.

## Produces
None

## Consumes
None

## Relevant Files
Read: `Scripts/ui-test-helper.sh` — helper functions (Xvfb, screenshot, compare)
Read: `/reference-docs/reference-screenshots/prol2-main-ui.jpg` — reference screenshot
Create: `screenshots/task-264-rmse-results.txt` — measurement results

## Acceptance Criteria
- [ ] Run: `cat screenshots/task-264-rmse-results.txt` → Expected: file exists with full-image and control-strip RMSE values
- [ ] Run: `cd /workspace/M-LIM/build && ctest --output-on-failure` → Expected: all tests pass

## Tests
None

## Technical Details
Use the same methodology as task 256:
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
# control strip only
convert /tmp/mlim-crop.png -crop 900x130+0+370 +repage /tmp/mlim-strip.png
convert /tmp/ref-crop.png  -crop 900x130+0+370 +repage /tmp/ref-strip.png
compare -metric RMSE /tmp/mlim-strip.png /tmp/ref-strip.png /tmp/diff-strip.png 2>&1
stop_app
```

## Dependencies
Requires tasks 257, 258, 259, 260, 261, 262, 263
