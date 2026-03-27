# Task: RMSE Remeasure After Wave 15

## Description

Remeasure all RMSE sub-regions after wave 15 changes (level-meter-idle-ambient-fill,
waveform-idle-fill-simulation) using the CORRECT methodology. Record baseline measurements
to establish the new reference point for wave 16.

## Produces
None

## Consumes
None

## Relevant Files
Read: `screenshots/` — previous wave screenshots for comparison

## Acceptance Criteria
- [ ] Run: Full RMSE → Expected: results below 21.5% (improvement from 22.25%)
- [ ] Run: Wave RMSE → Expected: results below 20.5%
- [ ] Run: Right panel RMSE → Expected: results below 26% (down from 29.26%)
- [ ] Commit screenshots and results

## Tests
None

## Technical Details

**Correct RMSE measurement methodology (identical to wave 14):**
```bash
# Start Xvfb if not running
DISPLAY=:99 xdpyinfo 2>/dev/null || (Xvfb :99 -screen 0 1920x1080x24 & sleep 2)

# Launch app
DISPLAY=:99 /workspace/M-LIM/build/MLIM_artefacts/Release/Standalone/M-LIM &
APP_PID=$!
sleep 8

# Full screenshot
DISPLAY=:99 scrot /tmp/mlim-wave15-full.png
kill $APP_PID

# M-LIM crop
convert /tmp/mlim-wave15-full.png -crop 908x500+509+325 +repage -resize 900x500! /tmp/mlim-w15.png

# Reference crop
convert /reference-docs/reference-screenshots/prol2-main-ui.jpg -crop 1712x1073+97+32 +repage -resize 900x500! /tmp/ref-w15.png

# Full RMSE
FULL=$(compare -metric RMSE /tmp/mlim-w15.png /tmp/ref-w15.png /dev/null 2>&1 | grep -oP '\d+\.\d+(?=\s*\()')
echo "Full: $FULL"

# Wave region (left 640px)
convert /tmp/mlim-w15.png -crop 640x500+0+0 +repage /tmp/mlim-w15-wave.png
convert /tmp/ref-w15.png  -crop 640x500+0+0 +repage /tmp/ref-w15-wave.png
WAVE=$(compare -metric RMSE /tmp/mlim-w15-wave.png /tmp/ref-w15-wave.png /dev/null 2>&1 | grep -oP '\d+\.\d+(?=\s*\()')
echo "Wave: $WAVE"

# Left meter region (80px, x=640-720)
convert /tmp/mlim-w15.png -crop 80x500+640+0 +repage /tmp/mlim-w15-left.png
convert /tmp/ref-w15.png  -crop 80x500+640+0 +repage /tmp/ref-w15-left.png
LEFT=$(compare -metric RMSE /tmp/mlim-w15-left.png /tmp/ref-w15-left.png /dev/null 2>&1 | grep -oP '\d+\.\d+(?=\s*\()')
echo "Left: $LEFT"

# Right panel (180px, x=720-900)
convert /tmp/mlim-w15.png -crop 180x500+720+0 +repage /tmp/mlim-w15-right.png
convert /tmp/ref-w15.png  -crop 180x500+720+0 +repage /tmp/ref-w15-right.png
RIGHT=$(compare -metric RMSE /tmp/mlim-w15-right.png /tmp/ref-w15-right.png /dev/null 2>&1 | grep -oP '\d+\.\d+(?=\s*\()')
echo "Right: $RIGHT"

# Control strip (bottom 90px)
convert /tmp/mlim-w15.png -crop 900x90+0+410 +repage /tmp/mlim-w15-ctrl.png
convert /tmp/ref-w15.png  -crop 900x90+0+410 +repage /tmp/ref-w15-ctrl.png
CTRL=$(compare -metric RMSE /tmp/mlim-w15-ctrl.png /tmp/ref-w15-ctrl.png /dev/null 2>&1 | grep -oP '\d+\.\d+(?=\s*\()')
echo "Control: $CTRL"
```

Copy results screenshot to `screenshots/task-NNN-wave15-crop.png` and save text results.

## Dependencies
Requires level-meter-idle-ambient-fill and waveform-idle-fill-simulation tasks to be completed first.
