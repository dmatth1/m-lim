# Task 411: RMSE Wave 22 — Closing Baseline Measurement

## Description

After all wave-22 tasks complete (403–410: waveform upper idle fill, midzone tent alpha
reduction, lower brightness boost, idle fill alpha tune, gradient top warm shift,
displayGradientBottom warmth, algoButtonInactive lightening, loudnessPanelBackground
lightening), capture a new screenshot and measure RMSE across all standard regions to
establish the wave-22 closing baseline.

Wave-21 baseline for comparison (from task 397):
- Full:    19.46%
- Wave:    16.72%
- Left:    28.71%
- Right:   23.09%
- Control: 21.02%

## Produces
None

## Consumes
None

## Relevant Files
Read: `Scripts/ui-test-helper.sh` — helper script reference
Read: `screenshots/task-397-rmse-results.txt` — wave-21 baseline
Modify: `screenshots/task-411-rmse-results.txt` — create closing baseline file

## Acceptance Criteria
- [ ] Run: `cmake --build build --target MLIM_Standalone_Standalone -j$(nproc)` → Expected: exit 0
- [ ] Run full RMSE → Expected: Full RMSE ≤ 19.46% (wave-21 full baseline)
- [ ] Run wave region RMSE → Expected: ≤ 16.72%
- [ ] Run left meters RMSE → Expected: ≤ 28.71%
- [ ] Run right panel RMSE → Expected: ≤ 23.09%
- [ ] Run control strip RMSE → Expected: ≤ 21.02%
- [ ] Run: `ls screenshots/task-411-rmse-results.txt screenshots/task-411-wave22-crop.png` → Expected: files exist
- [ ] Run: `git log --oneline -3` → Expected: baseline commit visible

## Tests
None

## Technical Details

```bash
# Step 1: Process reference
convert /reference-docs/reference-screenshots/prol2-main-ui.jpg \
    -crop 1712x1073+97+32 +repage -resize 900x500! /tmp/refW22.png

# Step 2: Build
export CCACHE_DIR=/build-cache
cmake --build /workspace/M-LIM/build --target MLIM_Standalone_Standalone -j$(nproc)

# Step 3: Launch and screenshot
pkill -f "M-LIM" 2>/dev/null; sleep 1
DISPLAY=:99 /workspace/M-LIM/build/MLIM_artefacts/Release/Standalone/M-LIM &>/dev/null &
sleep 8
DISPLAY=:99 scrot /tmp/task-w22-raw.png
pkill -f "M-LIM"

# Step 4: Crop
convert /tmp/task-w22-raw.png -crop 908x500+509+325 +repage -resize 900x500! /tmp/task-w22-mlim.png
cp /tmp/task-w22-mlim.png screenshots/task-411-wave22-crop.png

# Step 5: Full RMSE
echo "=== Full image RMSE ===" && compare -metric RMSE /tmp/refW22.png /tmp/task-w22-mlim.png /dev/null 2>&1

# Step 6: Sub-regions
convert /tmp/task-w22-mlim.png -crop 640x500+0+0 +repage /tmp/w22-wave.png
convert /tmp/refW22.png         -crop 640x500+0+0 +repage /tmp/ref-wave.png
echo "=== Wave region ===" && compare -metric RMSE /tmp/w22-wave.png /tmp/ref-wave.png /dev/null 2>&1

convert /tmp/task-w22-mlim.png -crop 80x500+640+0 +repage /tmp/w22-left.png
convert /tmp/refW22.png         -crop 80x500+640+0 +repage /tmp/ref-left.png
echo "=== Left meters ===" && compare -metric RMSE /tmp/w22-left.png /tmp/ref-left.png /dev/null 2>&1

convert /tmp/task-w22-mlim.png -crop 180x500+720+0 +repage /tmp/w22-right.png
convert /tmp/refW22.png         -crop 180x500+720+0 +repage /tmp/ref-right.png
echo "=== Right panel ===" && compare -metric RMSE /tmp/w22-right.png /tmp/ref-right.png /dev/null 2>&1

convert /tmp/task-w22-mlim.png -crop 900x90+0+410 +repage /tmp/w22-ctrl.png
convert /tmp/refW22.png         -crop 900x90+0+410 +repage /tmp/ref-ctrl.png
echo "=== Control strip ===" && compare -metric RMSE /tmp/w22-ctrl.png /tmp/ref-ctrl.png /dev/null 2>&1
```

Save all output to `screenshots/task-411-rmse-results.txt` and commit.

## Dependencies
Requires tasks 403, 404, 405, 406, 407, 408, 409, 410
