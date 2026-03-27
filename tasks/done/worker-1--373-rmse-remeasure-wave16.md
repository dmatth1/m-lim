# Task 373: RMSE Remeasure — Wave 16 Baseline

## Description
Measure full and sub-region RMSE after all wave 16 tasks complete. Establish new baselines
for all sub-regions and validate improvement/regression against wave 15 state.

**Wave 16 tasks expected:**
- Fix left meter regression: loudness panel histogram brightness
- Level meter idle gradient top zone fix

**Wave 15 baseline (task-370):**
- Full: 21.30%
- Wave: 19.44%
- Left meters: 28.11% (regression to fix)
- Right panel: 23.94%
- Control strip: 20.65%

## Produces
None

## Consumes
None

## Relevant Files
Read: `screenshots/task-370-rmse-results.txt` — wave 15 baseline

## Acceptance Criteria
- [x] Run: Full RMSE measurement → saved to screenshots/task-373-rmse-results.txt ✓
- [x] Full image RMSE: 21.22% ✓ (≤21.30%, improved -0.08pp)
- [BLOCKED] Left meter RMSE: 28.11% ✗ (target ≤22.00% not met — structural layout floor)
- [x] Right panel RMSE: 23.57% ✓ (≤24.50%, improved from 23.94%)

## Tests
None

## Technical Details

```bash
# Step 1: Prepare reference
convert /reference-docs/reference-screenshots/prol2-main-ui.jpg \
    -crop 1712x1073+97+32 +repage -resize 900x500! /tmp/refW16.png

# Step 2: Build Standalone only
export CCACHE_DIR=/build-cache
cmake --build build --target MLIM_Standalone -j$(nproc)

# Step 3: Launch and capture
pkill -f "Standalone/M-LIM" 2>/dev/null; sleep 1
DISPLAY=:99 /workspace/M-LIM/build/MLIM_artefacts/Release/Standalone/M-LIM &
APP_PID=$!
sleep 8
DISPLAY=:99 scrot /tmp/task-w16-raw.png
kill $APP_PID

# Step 4: Crop to plugin area
convert /tmp/task-w16-raw.png -crop 908x500+509+325 +repage -resize 900x500! /tmp/task-w16-mlim.png

# Step 5: Full RMSE
echo "=== Full image RMSE ==="
compare -metric RMSE /tmp/refW16.png /tmp/task-w16-mlim.png /dev/null 2>&1

# Step 6: Sub-region RMSE
# Waveform (left 640px)
convert /tmp/task-w16-mlim.png -crop 640x500+0+0 +repage /tmp/w16-wave.png
convert /tmp/refW16.png         -crop 640x500+0+0 +repage /tmp/ref-wave.png
echo "=== Wave region ==="
compare -metric RMSE /tmp/w16-wave.png /tmp/ref-wave.png /dev/null 2>&1

# Left meters (80px, x=640-720)
convert /tmp/task-w16-mlim.png -crop 80x500+640+0 +repage /tmp/w16-left.png
convert /tmp/refW16.png         -crop 80x500+640+0 +repage /tmp/ref-left.png
echo "=== Left meters ==="
compare -metric RMSE /tmp/w16-left.png /tmp/ref-left.png /dev/null 2>&1

# Right panel (180px, x=720-900)
convert /tmp/task-w16-mlim.png -crop 180x500+720+0 +repage /tmp/w16-right.png
convert /tmp/refW16.png         -crop 180x500+720+0 +repage /tmp/ref-right.png
echo "=== Right panel ==="
compare -metric RMSE /tmp/w16-right.png /tmp/ref-right.png /dev/null 2>&1

# Control strip (bottom 90px)
convert /tmp/task-w16-mlim.png -crop 900x90+0+410 +repage /tmp/w16-ctrl.png
convert /tmp/refW16.png         -crop 900x90+0+410 +repage /tmp/ref-ctrl.png
echo "=== Control strip ==="
compare -metric RMSE /tmp/w16-ctrl.png /tmp/ref-ctrl.png /dev/null 2>&1
```

Save text results to `screenshots/task-NNN-rmse-results.txt`.
Save plugin screenshot to `screenshots/task-NNN-wave16-crop.png`.

## Dependencies
Requires tasks 371, 372
