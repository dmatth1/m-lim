# Task 393: RMSE Remeasure — Wave 20 Baseline

## Description

After wave-20 tasks complete (waveform bottom gradient brightening, waveform mid-zone
boost, waveform top warm shift, control strip additional brightening), capture a new
screenshot and measure RMSE across all standard regions to establish the wave-20 baseline.

Wave-19 baseline for comparison (from task 388):
- Full: TBD (≤ 20.74% target)
- Wave: TBD (≤ 18.96% target)
- Left: TBD (structural floor ~29%)
- Right: TBD (≤ 22.36% target)
- Control: TBD (≤ 20.17% target)

## Produces
None

## Consumes
None

## Relevant Files
Read: `Scripts/ui-test-helper.sh` — helper script reference
Read: `screenshots/task-388-rmse-results.txt` — wave-19 baseline for comparison

## Acceptance Criteria
- [ ] Run: full image RMSE → Expected: ≤ wave-19 full baseline (no regression)
- [ ] Run: wave region RMSE → Expected: ≤ wave-19 wave baseline (no regression)
- [ ] Run: right region RMSE → Expected: ≤ wave-19 right baseline (no regression)
- [ ] Save results to `screenshots/task-393-rmse-results.txt`
- [ ] Save screenshot crop to `screenshots/task-393-wave20-crop.png`

## Tests
None

## Technical Details

```bash
# Step 1: Process reference image
convert /reference-docs/reference-screenshots/prol2-main-ui.jpg \
    -crop 1712x1073+97+32 +repage -resize 900x500! /tmp/refW20.png

# Step 2: Build Standalone only
export CCACHE_DIR=/build-cache
cmake --build /workspace/M-LIM/build --target MLIM_Standalone_Standalone -j$(nproc)

# Step 3: Launch and screenshot
pkill -f "Standalone/M-LIM" 2>/dev/null; sleep 1
DISPLAY=:99 /workspace/M-LIM/build/MLIM_artefacts/Release/Standalone/M-LIM &>/dev/null &
sleep 8
DISPLAY=:99 scrot /tmp/task-w20-raw.png
pkill -f "Standalone/M-LIM"

# Step 4: Crop to standard size
convert /tmp/task-w20-raw.png -crop 908x500+509+325 +repage -resize 900x500! /tmp/task-w20-mlim.png

# Step 5: Full RMSE
echo "=== Full image RMSE ==="
compare -metric RMSE /tmp/refW20.png /tmp/task-w20-mlim.png /dev/null 2>&1

# Step 6: Sub-region RMSE
convert /tmp/task-w20-mlim.png -crop 640x500+0+0 +repage /tmp/w20-wave.png
convert /tmp/refW20.png         -crop 640x500+0+0 +repage /tmp/ref-wave.png
echo "=== Wave region ==="
compare -metric RMSE /tmp/w20-wave.png /tmp/ref-wave.png /dev/null 2>&1

convert /tmp/task-w20-mlim.png -crop 80x500+640+0 +repage /tmp/w20-left.png
convert /tmp/refW20.png         -crop 80x500+640+0 +repage /tmp/ref-left.png
echo "=== Left meters ==="
compare -metric RMSE /tmp/w20-left.png /tmp/ref-left.png /dev/null 2>&1

convert /tmp/task-w20-mlim.png -crop 180x500+720+0 +repage /tmp/w20-right.png
convert /tmp/refW20.png         -crop 180x500+720+0 +repage /tmp/ref-right.png
echo "=== Right panel ==="
compare -metric RMSE /tmp/w20-right.png /tmp/ref-right.png /dev/null 2>&1

convert /tmp/task-w20-mlim.png -crop 900x90+0+410 +repage /tmp/w20-ctrl.png
convert /tmp/refW20.png         -crop 900x90+0+410 +repage /tmp/ref-ctrl.png
echo "=== Control strip ==="
compare -metric RMSE /tmp/w20-ctrl.png /tmp/ref-ctrl.png /dev/null 2>&1
```

Save text results to `screenshots/task-393-rmse-results.txt` (replace NNN with 393).
Save `screenshots/task-393-wave20-crop.png` (the plugin crop image).

Commit: `worker-N: complete task 393 - wave 20 RMSE baseline (full X%, wave Y%, right Z%)`

## Dependencies
Requires tasks 389, 390, 391, 392
