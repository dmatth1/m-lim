# Task: RMSE Remeasure — Wave 19 Baseline

## Description

After completing all wave-19 visual parity tasks (waveform center brightness boost,
LUFS panel background brightness, level meter warm zone increase, control strip gradient
tune, and any other wave-19 fixes), capture a new screenshot and measure RMSE across
all standard regions to establish the wave 19 baseline.

Wave 18 baseline for comparison:
- Full: 20.74%
- Wave: 18.96%
- Left: 29.00% (structural floor)
- Right: 22.36%
- Control: 20.17%

## Produces
None

## Consumes
None

## Relevant Files
Read: `Scripts/ui-test-helper.sh` — helper script reference
Read: `screenshots/task-383-rmse-results.txt` — wave 18 baseline for comparison

## Acceptance Criteria
- [ ] Run: full image RMSE → Expected: ≤ 20.74% (at least matches wave 18)
- [ ] Run: wave region RMSE → Expected: ≤ 18.96% (at least matches wave 18)
- [ ] Run: right region RMSE → Expected: ≤ 22.36% (at least matches wave 18)
- [ ] Save results to `screenshots/task-NNN-rmse-results.txt`
- [ ] Save screenshot crop to `screenshots/task-NNN-wave19-crop.png`

## Tests
None

## Technical Details

```bash
# Step 1: Process reference image
convert /reference-docs/reference-screenshots/prol2-main-ui.jpg \
    -crop 1712x1073+97+32 +repage -resize 900x500! /tmp/refW19.png

# Step 2: Build Standalone only
export CCACHE_DIR=/build-cache
cmake --build /workspace/M-LIM/build --target MLIM_Standalone -j$(nproc)

# Step 3: Launch and screenshot
pkill -f "Standalone/M-LIM" 2>/dev/null; sleep 1
DISPLAY=:99 /workspace/M-LIM/build/MLIM_artefacts/Release/Standalone/M-LIM &>/dev/null &
sleep 8
DISPLAY=:99 scrot /tmp/task-w19-raw.png
pkill -f "Standalone/M-LIM"

# Step 4: Crop to standard size
convert /tmp/task-w19-raw.png -crop 908x500+509+325 +repage -resize 900x500! /tmp/task-w19-mlim.png

# Step 5: Full RMSE
echo "=== Full image RMSE ==="
compare -metric RMSE /tmp/refW19.png /tmp/task-w19-mlim.png /dev/null 2>&1

# Step 6: Sub-region RMSE
convert /tmp/task-w19-mlim.png -crop 640x500+0+0 +repage /tmp/w19-wave.png
convert /tmp/refW19.png         -crop 640x500+0+0 +repage /tmp/ref-wave.png
echo "=== Wave region ==="
compare -metric RMSE /tmp/w19-wave.png /tmp/ref-wave.png /dev/null 2>&1

convert /tmp/task-w19-mlim.png -crop 80x500+640+0 +repage /tmp/w19-left.png
convert /tmp/refW19.png         -crop 80x500+640+0 +repage /tmp/ref-left.png
echo "=== Left meters ==="
compare -metric RMSE /tmp/w19-left.png /tmp/ref-left.png /dev/null 2>&1

convert /tmp/task-w19-mlim.png -crop 180x500+720+0 +repage /tmp/w19-right.png
convert /tmp/refW19.png         -crop 180x500+720+0 +repage /tmp/ref-right.png
echo "=== Right panel ==="
compare -metric RMSE /tmp/w19-right.png /tmp/ref-right.png /dev/null 2>&1

convert /tmp/task-w19-mlim.png -crop 900x90+0+410 +repage /tmp/w19-ctrl.png
convert /tmp/refW19.png         -crop 900x90+0+410 +repage /tmp/ref-ctrl.png
echo "=== Control strip ==="
compare -metric RMSE /tmp/w19-ctrl.png /tmp/ref-ctrl.png /dev/null 2>&1
```

Save text results to `screenshots/task-NNN-rmse-results.txt` (replace NNN with task number).
Save `screenshots/task-NNN-wave19-crop.png` (the plugin crop image).

Commit: `worker-N: complete task NNN - wave 19 RMSE baseline (full X%, wave Y%, right Z%)`

## Dependencies
Requires all wave 19 fix tasks to be completed first.
