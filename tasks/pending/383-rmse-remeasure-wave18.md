# Task 383: RMSE Remeasure — Wave 18 Baseline

## Description

Measure full and sub-region RMSE after all wave 18 tasks complete. Establish new baselines
and validate improvement/regression against wave 17 state.

**Wave 18 tasks:**
- Task 378: Darken waveform gradient top (displayGradientTop → 0xff242027)
- Task 379: Increase waveform idle fill opacity (0.62f → 0.80f)
- Task 380: Reduce waveform grid line alpha (0.6f → 0.35f)
- Task 381: Brighten rotary knob face (knobFaceHighlight/Shadow)
- Task 382: Reduce level meter idle bottom glow (0.44f → 0.08f)

**Wave 17 baseline (task-377):**
- Full:    (from task-377 results)
- Wave:    (from task-377 results)
- Left:    28.11% (structural floor — not targeted in wave 18)
- Right:   (from task-377 results)
- Control: (from task-377 results)

## Produces
None

## Consumes
None

## Relevant Files
Read: `screenshots/task-377-rmse-results.txt` — wave 17 RMSE baseline

## Acceptance Criteria
- [ ] Run: full RMSE measurement → save to `screenshots/task-383-rmse-results.txt`
- [ ] Full image RMSE: ≤ wave-17 full baseline (no regression)
- [ ] Wave region RMSE: improvement expected (tasks 378, 379, 380)
- [ ] Right panel RMSE: improvement expected (task 382)
- [ ] Control strip RMSE: improvement expected (task 381)

## Tests
None

## Technical Details

```bash
# Step 1: Prepare reference
convert /reference-docs/reference-screenshots/prol2-main-ui.jpg \
    -crop 1712x1073+97+32 +repage -resize 900x500! /tmp/refW18.png

# Step 2: Build Standalone only
export CCACHE_DIR=/build-cache
cmake --build /workspace/M-LIM/build --target MLIM_Standalone_Standalone -j$(nproc)

# Step 3: Launch and capture
pkill -f "Standalone/M-LIM" 2>/dev/null; sleep 1
DISPLAY=:99 /workspace/M-LIM/build/MLIM_artefacts/Release/Standalone/M-LIM &>/dev/null &
APP_PID=$!
sleep 8
DISPLAY=:99 scrot /tmp/task-w18-raw.png
kill $APP_PID 2>/dev/null

# Step 4: Crop to plugin area
convert /tmp/task-w18-raw.png -crop 908x500+509+325 +repage -resize 900x500! /tmp/task-w18-mlim.png

# Step 5: Full RMSE
echo "=== Full image RMSE ==="
compare -metric RMSE /tmp/refW18.png /tmp/task-w18-mlim.png /dev/null 2>&1

# Step 6: Sub-region RMSE
convert /tmp/task-w18-mlim.png -crop 640x500+0+0 +repage /tmp/w18-wave.png
convert /tmp/refW18.png         -crop 640x500+0+0 +repage /tmp/ref-wave.png
echo "=== Wave region ==="
compare -metric RMSE /tmp/w18-wave.png /tmp/ref-wave.png /dev/null 2>&1

convert /tmp/task-w18-mlim.png -crop 80x500+640+0 +repage /tmp/w18-left.png
convert /tmp/refW18.png         -crop 80x500+640+0 +repage /tmp/ref-left.png
echo "=== Left meters ==="
compare -metric RMSE /tmp/w18-left.png /tmp/ref-left.png /dev/null 2>&1

convert /tmp/task-w18-mlim.png -crop 180x500+720+0 +repage /tmp/w18-right.png
convert /tmp/refW18.png         -crop 180x500+720+0 +repage /tmp/ref-right.png
echo "=== Right panel ==="
compare -metric RMSE /tmp/w18-right.png /tmp/ref-right.png /dev/null 2>&1

convert /tmp/task-w18-mlim.png -crop 900x90+0+410 +repage /tmp/w18-ctrl.png
convert /tmp/refW18.png         -crop 900x90+0+410 +repage /tmp/ref-ctrl.png
echo "=== Control strip ==="
compare -metric RMSE /tmp/w18-ctrl.png /tmp/ref-ctrl.png /dev/null 2>&1
```

Save text results to `screenshots/task-383-rmse-results.txt`.
Save plugin crop to `screenshots/task-383-wave18-crop.png`.

## Dependencies
Requires tasks 378, 379, 380, 381, 382
