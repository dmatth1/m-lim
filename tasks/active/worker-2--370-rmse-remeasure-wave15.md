# Task 370: RMSE Remeasure — Wave 15 Baseline

## Description
Measure full and sub-region RMSE after all wave 15 tasks complete:
- Task 365: Level meter idle ambient fill (boost idle gradient brightness)
- Task 366: Output meter track warm background (barTrackBackground warmth fix)
- Task 367: LUFS histogram waveform gradient background
- Task 368: Waveform gradient darken top
- Task 369: Waveform idle fill simulation

Establish new baselines for all sub-regions and validate improvements against wave 14 state.

## Produces
None

## Consumes
None

## Relevant Files
Read: `screenshots/task-364-rmse-results.txt` — wave 14 baseline
Read: `Scripts/ui-test-helper.sh` — screenshot capture methodology

## Acceptance Criteria
- [ ] Run: Full RMSE measurement → Expected: results saved to `screenshots/task-370-rmse-results.txt`
- [ ] Full image RMSE reported → Expected: ≤ 21.50% (improvement from 22.25% wave 14)
- [ ] Waveform region RMSE reported → Expected: ≤ 20.50% (improvement from 21.45% wave 14)
- [ ] Right panel RMSE reported → Expected: ≤ 28.00% (improvement from 29.26% wave 14)

## Tests
None

## Technical Details

**CORRECT measurement methodology:**

```bash
# Step 1: Prepare reference
convert /reference-docs/reference-screenshots/prol2-main-ui.jpg \
    -crop 1712x1073+97+32 +repage -resize 900x500! /tmp/refW15.png

# Step 2: Build Standalone only
export CCACHE_DIR=/build-cache
cmake --build build --target MLIM_Standalone_Standalone \
    -DCMAKE_CXX_COMPILER_LAUNCHER=ccache \
    -DCCACHE_SLOPPINESS=pch_defines,time_macros,include_file_mtime,include_file_ctime \
    -j$(nproc)

# Step 3: Launch and capture
Xvfb :99 -screen 0 1920x1080x24 &
sleep 2
export DISPLAY=:99
/workspace/M-LIM/build/MLIM_artefacts/Release/Standalone/M-LIM &
APP_PID=$!
sleep 8
scrot /tmp/task370-raw.png
kill $APP_PID

# Step 4: Crop to plugin area (CORRECT methodology)
convert /tmp/task370-raw.png -crop 908x500+509+325 +repage -resize 900x500! /tmp/task370-mlim.png

# Step 5: Full image RMSE
echo "=== Full image RMSE ==="
compare -metric RMSE /tmp/refW15.png /tmp/task370-mlim.png /dev/null 2>&1

# Step 6: Sub-region RMSE
# Waveform (left 640px)
convert /tmp/task370-mlim.png -crop 640x500+0+0 +repage /tmp/t370-wave.png
convert /tmp/refW15.png        -crop 640x500+0+0 +repage /tmp/ref-wave.png
echo "=== Wave region ==="
compare -metric RMSE /tmp/t370-wave.png /tmp/ref-wave.png /dev/null 2>&1

# Left meters (80px, x=640-720)
convert /tmp/task370-mlim.png -crop 80x500+640+0 +repage /tmp/t370-left.png
convert /tmp/refW15.png        -crop 80x500+640+0 +repage /tmp/ref-left.png
echo "=== Left meters ==="
compare -metric RMSE /tmp/t370-left.png /tmp/ref-left.png /dev/null 2>&1

# Right panel (180px, x=720-900)
convert /tmp/task370-mlim.png -crop 180x500+720+0 +repage /tmp/t370-right.png
convert /tmp/refW15.png        -crop 180x500+720+0 +repage /tmp/ref-right.png
echo "=== Right panel ==="
compare -metric RMSE /tmp/t370-right.png /tmp/ref-right.png /dev/null 2>&1

# Control strip (bottom 90px)
convert /tmp/task370-mlim.png -crop 900x90+0+410 +repage /tmp/t370-ctrl.png
convert /tmp/refW15.png        -crop 900x90+0+410 +repage /tmp/ref-ctrl.png
echo "=== Control strip ==="
compare -metric RMSE /tmp/t370-ctrl.png /tmp/ref-ctrl.png /dev/null 2>&1
```

**Wave 14 baseline (task-364):**
- Full image:    22.25%
- Waveform:      21.45%  (wave region)
- Left meters:   20.85%
- Right panel:   29.26%  ← biggest gap
- Control strip: 20.74%

Copy plugin screenshot to `screenshots/task-370-wave15-crop.png` and save text results to
`screenshots/task-370-rmse-results.txt`.

## Dependencies
Requires tasks 365, 366, 367, 368, 369
