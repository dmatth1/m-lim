# Task 377: RMSE Remeasure — Wave 17 Baseline

## Description

Measure full and sub-region RMSE after all wave 17 tasks complete. Establish new baselines
and validate improvement/regression against wave 16 state.

**Wave 17 tasks:**
- Task 374: Right panel histogram warm dark background (loudnessHistogramTop/Bottom)
- Task 375: Control strip gradient brightness increase (controlStripTop/Bottom)
- Task 376: Waveform idle fill extend coverage to mid-zone (new mid gradient block)

**Wave 16 baseline (task-373):**
- Full:    21.22%
- Wave:    19.44%
- Left:    28.11% (structural floor — not targeted in wave 17)
- Right:   23.57%
- Control: 20.65%

## Produces
None

## Consumes
None

## Relevant Files
Read: `M-LIM/screenshots/sweep19-rmse-baseline.txt` — wave 16 RMSE baseline

## Acceptance Criteria
- [ ] Run: full RMSE measurement → save to `screenshots/task-377-rmse-results.txt`
- [ ] Full image RMSE: ≤ 21.22% (no regression from wave 16)
- [ ] Wave region RMSE: ≤ 19.44% (improvement expected from task 376)
- [ ] Right panel RMSE: ≤ 23.57% (improvement expected from task 374)
- [ ] Control strip RMSE: ≤ 20.65% (improvement expected from task 375)

## Tests
None

## Technical Details

```bash
# Step 1: Prepare reference
convert /reference-docs/reference-screenshots/prol2-main-ui.jpg \
    -crop 1712x1073+97+32 +repage -resize 900x500! /tmp/refW17.png

# Step 2: Build Standalone only
export CCACHE_DIR=/build-cache
cmake --build /workspace/M-LIM/build --target MLIM_Standalone_Standalone -j$(nproc)

# Step 3: Launch and capture
pkill -f "Standalone/M-LIM" 2>/dev/null; sleep 1
DISPLAY=:99 /workspace/M-LIM/build/MLIM_artefacts/Release/Standalone/M-LIM &>/dev/null &
APP_PID=$!
sleep 8
DISPLAY=:99 scrot /tmp/task-w17-raw.png
kill $APP_PID 2>/dev/null

# Step 4: Crop to plugin area
convert /tmp/task-w17-raw.png -crop 908x500+509+325 +repage -resize 900x500! /tmp/task-w17-mlim.png

# Step 5: Full RMSE
echo "=== Full image RMSE ==="
compare -metric RMSE /tmp/refW17.png /tmp/task-w17-mlim.png /dev/null 2>&1

# Step 6: Sub-region RMSE
convert /tmp/task-w17-mlim.png -crop 640x500+0+0 +repage /tmp/w17-wave.png
convert /tmp/refW17.png         -crop 640x500+0+0 +repage /tmp/ref-wave.png
echo "=== Wave region ==="
compare -metric RMSE /tmp/w17-wave.png /tmp/ref-wave.png /dev/null 2>&1

convert /tmp/task-w17-mlim.png -crop 80x500+640+0 +repage /tmp/w17-left.png
convert /tmp/refW17.png         -crop 80x500+640+0 +repage /tmp/ref-left.png
echo "=== Left meters ==="
compare -metric RMSE /tmp/w17-left.png /tmp/ref-left.png /dev/null 2>&1

convert /tmp/task-w17-mlim.png -crop 180x500+720+0 +repage /tmp/w17-right.png
convert /tmp/refW17.png         -crop 180x500+720+0 +repage /tmp/ref-right.png
echo "=== Right panel ==="
compare -metric RMSE /tmp/w17-right.png /tmp/ref-right.png /dev/null 2>&1

convert /tmp/task-w17-mlim.png -crop 900x90+0+410 +repage /tmp/w17-ctrl.png
convert /tmp/refW17.png         -crop 900x90+0+410 +repage /tmp/ref-ctrl.png
echo "=== Control strip ==="
compare -metric RMSE /tmp/w17-ctrl.png /tmp/ref-ctrl.png /dev/null 2>&1
```

Save text results to `screenshots/task-377-rmse-results.txt`.
Save plugin crop to `screenshots/task-377-wave17-crop.png`.

## Dependencies
Requires tasks 374, 375, 376
