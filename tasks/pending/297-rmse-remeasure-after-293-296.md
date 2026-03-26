# Task 297: RMSE Re-Measure After Tasks 293–296

## Description
After visual parity tasks 293 (ADVANCED button position fix), 294 (remove left input meter), 295 (waveform gradient darken), and 296 (level meter visual improvements) are complete, measure updated RMSE to quantify progress and identify next improvements.

**Steps:**
1. Build latest code.
2. Launch standalone on Xvfb :99.
3. Use the standard measurement methodology from task 288:
   - Crop plugin editor (skip 64px title bar)
   - Resize to 900x500
   - Crop reference `-crop 1712x1073+97+32`, resize to 900x500
4. Run `compare -metric RMSE` on full image and sub-regions.
5. Document results and list top remaining visual differences.
6. Save to `screenshots/task-297-rmse-results.txt`.

**Baseline from task 288:**
- Full image: 25.70% (0.2570)
- Control strip: 23.26%
- Waveform: 25.79%
- Left level meter: 33.36% (regressed — target fix via task 293/294)
- Right panel: 22.78%

**Target after tasks 293–296:** full image RMSE ≤ 23% (0.23).

## Produces
None

## Consumes
None

## Relevant Files
Read: `/reference-docs/reference-screenshots/prol2-main-ui.jpg` — reference
Read: `screenshots/task-288-rmse-results.txt` — baseline
Create: `screenshots/task-297-rmse-results.txt` — new results

## Acceptance Criteria
- [ ] Run: `cat screenshots/task-297-rmse-results.txt` → Expected: file exists with full-image and sub-region RMSE values
- [ ] Run: full-image RMSE on 900x500 → Expected: ≤ 0.25 (improvement from 0.2570 baseline)
- [ ] Run: left-meter RMSE `30x378+0+30` → Expected: ≤ 0.30 (recovery from 0.3336 regression)

## Tests
None

## Technical Details
```bash
# Standard measurement (same as task 288)
WID=$(DISPLAY=:99 xdotool search --name "M-LIM" | head -1)
GEOM=$(DISPLAY=:99 xdotool getwindowgeometry $WID)
# Parse X, Y, W from GEOM, then:
convert /tmp/mlim-raw.png -crop ${W}x500+${X}+$((Y+64)) +repage /tmp/mlim-crop.png
convert /tmp/mlim-crop.png -resize 900x500! /tmp/mlim-900.png
convert /reference-docs/reference-screenshots/prol2-main-ui.jpg \
  -crop 1712x1073+97+32 +repage -resize 900x500! /tmp/ref-900.png
compare -metric RMSE /tmp/mlim-900.png /tmp/ref-900.png /tmp/diff-full.png 2>&1
```

## Dependencies
Requires tasks 293, 294, 295, 296
