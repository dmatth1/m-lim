# Task: RMSE Remeasure — Wave 21 Baseline

## Description

After wave-21 tasks complete (waveform mid-zone boost shift down, gradient bottom
blue boost, waveform top gradient darken), capture a new screenshot and measure
RMSE across all standard regions to establish the wave-21 baseline.

Wave-20 baseline for comparison (from task 393):
- Full:    19.82%
- Wave:    17.29%
- Left:    28.75%
- Right:   23.09%
- Control: 21.02%

## Produces
None

## Consumes
None

## Relevant Files
Read: `Scripts/ui-test-helper.sh` — helper script reference
Read: `screenshots/task-393-rmse-results.txt` — wave-20 baseline (if it exists)

## Acceptance Criteria
- [ ] Run: full image RMSE → Expected: ≤ wave-20 full baseline 19.82%
- [ ] Run: wave region RMSE → Expected: ≤ wave-20 wave baseline 17.29%
- [ ] Run: right region RMSE → Expected: ≤ wave-20 right baseline 23.09%
- [ ] Save results to `screenshots/task-NNN-rmse-results.txt`
- [ ] Save screenshot crop to `screenshots/task-NNN-wave21-crop.png`

## Tests
None

## Technical Details

```bash
# Step 1: Process reference image
convert /reference-docs/reference-screenshots/prol2-main-ui.jpg \
    -crop 1712x1073+97+32 +repage -resize 900x500! /tmp/refW21.png

# Step 2: Build Standalone only
export CCACHE_DIR=/build-cache
cmake --build /workspace/M-LIM/build --target MLIM_Standalone -j$(nproc)

# Step 3: Launch and screenshot
pkill -f "M-LIM" 2>/dev/null; sleep 1
DISPLAY=:99 /workspace/M-LIM/build/MLIM_artefacts/Release/Standalone/M-LIM &>/dev/null &
sleep 8
DISPLAY=:99 scrot /tmp/task-w21-raw.png
pkill -f "M-LIM"

# Step 4: Crop to standard size
convert /tmp/task-w21-raw.png -crop 908x500+509+325 +repage -resize 900x500! /tmp/task-w21-mlim.png

# Step 5: Full RMSE
echo "=== Full image RMSE ==="
compare -metric RMSE /tmp/refW21.png /tmp/task-w21-mlim.png /dev/null 2>&1

# Step 6: Sub-region RMSE
convert /tmp/task-w21-mlim.png -crop 640x500+0+0 +repage /tmp/w21-wave.png
convert /tmp/refW21.png         -crop 640x500+0+0 +repage /tmp/ref-wave.png
echo "=== Wave region ==="
compare -metric RMSE /tmp/w21-wave.png /tmp/ref-wave.png /dev/null 2>&1

convert /tmp/task-w21-mlim.png -crop 80x500+640+0 +repage /tmp/w21-left.png
convert /tmp/refW21.png         -crop 80x500+640+0 +repage /tmp/ref-left.png
echo "=== Left meters ==="
compare -metric RMSE /tmp/w21-left.png /tmp/ref-left.png /dev/null 2>&1

convert /tmp/task-w21-mlim.png -crop 180x500+720+0 +repage /tmp/w21-right.png
convert /tmp/refW21.png         -crop 180x500+720+0 +repage /tmp/ref-right.png
echo "=== Right panel ==="
compare -metric RMSE /tmp/w21-right.png /tmp/ref-right.png /dev/null 2>&1

convert /tmp/task-w21-mlim.png -crop 900x90+0+410 +repage /tmp/w21-ctrl.png
convert /tmp/refW21.png         -crop 900x90+0+410 +repage /tmp/ref-ctrl.png
echo "=== Control strip ==="
compare -metric RMSE /tmp/w21-ctrl.png /tmp/ref-ctrl.png /dev/null 2>&1
```

Save text results to `screenshots/task-NNN-rmse-results.txt`.
Save `screenshots/task-NNN-wave21-crop.png` (the plugin crop image).

Commit: `worker-N: complete task NNN - wave 21 RMSE baseline (full X%, wave Y%, right Z%)`

## Dependencies
Requires waveform-midzone-boost-shift-down, waveform-gradient-bottom-blue-boost, waveform-top-gradient-darken
