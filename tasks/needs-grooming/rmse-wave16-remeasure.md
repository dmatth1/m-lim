# Task: RMSE Remeasure — Wave 16 Baseline

## Description

Measure full and sub-region RMSE after all wave 16 tasks complete. Also, note that task 370
(wave 15 remeasure) used DIFFERENT sub-region crop coordinates than the established task-354
baseline. This task must use the CORRECT coordinates to establish a comparable baseline.

**Wave 15 coordinate error**: Task 370 measured "left meters" at `80x500+640+0` and "right
panel" at `180x500+720+0`. The correct baseline coordinates (task-354) are:
- Left meters: `30x378+0+30`
- Waveform: `600x400+150+50`
- Right panel: `100x400+800+50`
- Control strip: `900x60+0+440`

When measured with correct coordinates, the current state shows:
- Full: 21.30% (wave 15 measurement was correct)
- Left (correct coords): 21.0% — IMPROVED from 23.5% baseline (no regression!)
- Wave (correct coords): 20.4% — IMPROVED from 20.55% baseline
- Right (correct coords): 28.5% — IMPROVED from 29.59% baseline
- Control (correct coords): 20.7% — IMPROVED from 22.42% baseline

The "left meter regression +7.26pp" in the wave 15 harness note was a false alarm caused by
comparing the wrong coordinates.

## Produces
None

## Consumes
None

## Relevant Files
Read: `screenshots/task-354-rmse-results.txt` — correct baseline (if it exists) or use values above
Read: `Scripts/ui-test-helper.sh` — screenshot methodology

## Acceptance Criteria
- [ ] Run: Full RMSE measurement → Expected: results saved to `screenshots/task-NNN-rmse-results.txt`
- [ ] Full image RMSE → Expected: ≤ 20.80% (improvement from 21.30%)
- [ ] Right panel (correct coords: 100×400+800+50) → Expected: ≤ 27.00% (improvement from 28.5%)
- [ ] Left meters (correct coords: 30×378+0+30) → Expected: ≤ 20.50% (improvement from 21.0%)

## Tests
None

## Technical Details

**CORRECT measurement methodology:**

```bash
# Step 1: Prepare reference (SAME AS ALWAYS)
convert /reference-docs/reference-screenshots/prol2-main-ui.jpg \
    -crop 1712x1073+97+32 +repage -resize 900x500! /tmp/refW16.png

# Step 2: Build
export CCACHE_DIR=/build-cache
cmake --build /workspace/M-LIM/build --target MLIM_Standalone -j$(nproc)

# Step 3: Launch and capture
pkill -f "Standalone/M-LIM" 2>/dev/null; sleep 1
Xvfb :99 -screen 0 1920x1080x24 &
sleep 2
export DISPLAY=:99
/workspace/M-LIM/build/MLIM_artefacts/Release/Standalone/M-LIM &
APP_PID=$!
sleep 8
scrot /tmp/task-wave16-raw.png
kill $APP_PID

# Step 4: Crop to plugin area
convert /tmp/task-wave16-raw.png -crop 908x500+509+325 +repage -resize 900x500! /tmp/wave16-mlim.png

# Step 5: Full image RMSE
echo "=== Full image RMSE ==="
compare -metric RMSE /tmp/refW16.png /tmp/wave16-mlim.png /dev/null 2>&1

# Step 6: Sub-region RMSE with CORRECT baseline coordinates (matching task-354)
# Waveform (600x400, starting at 150,50)
convert /tmp/wave16-mlim.png -crop 600x400+150+50 +repage /tmp/w16-wave.png
convert /tmp/refW16.png       -crop 600x400+150+50 +repage /tmp/ref-wave.png
echo "=== Waveform region (correct coords) ==="
compare -metric RMSE /tmp/w16-wave.png /tmp/ref-wave.png /dev/null 2>&1

# Left meters (30x378, starting at 0,30)
convert /tmp/wave16-mlim.png -crop 30x378+0+30 +repage /tmp/w16-left.png
convert /tmp/refW16.png       -crop 30x378+0+30 +repage /tmp/ref-left.png
echo "=== Left meters (correct coords) ==="
compare -metric RMSE /tmp/w16-left.png /tmp/ref-left.png /dev/null 2>&1

# Right panel (100x400, starting at 800,50) — output meter area
convert /tmp/wave16-mlim.png -crop 100x400+800+50 +repage /tmp/w16-right.png
convert /tmp/refW16.png       -crop 100x400+800+50 +repage /tmp/ref-right.png
echo "=== Right panel (correct coords) ==="
compare -metric RMSE /tmp/w16-right.png /tmp/ref-right.png /dev/null 2>&1

# Control strip (900x60, starting at 0,440)
convert /tmp/wave16-mlim.png -crop 900x60+0+440 +repage /tmp/w16-ctrl.png
convert /tmp/refW16.png       -crop 900x60+0+440 +repage /tmp/ref-ctrl.png
echo "=== Control strip (correct coords) ==="
compare -metric RMSE /tmp/w16-ctrl.png /tmp/ref-ctrl.png /dev/null 2>&1
```

**Wave 15 state (measured by auditor with correct coords):**
- Full:         21.30%
- Waveform:     20.40%
- Left meters:  21.00%
- Right panel:  28.50%
- Control:      20.70%

Copy plugin screenshot to `screenshots/task-NNN-wave16-crop.png` and save text results.

## Dependencies
Requires: revert-loudness-panel-waveform-gradient, output-meter-idle-gradient-rework
