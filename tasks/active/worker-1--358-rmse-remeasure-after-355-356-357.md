# Task 358: RMSE Remeasure After Tasks 355, 356, 357

## Description

Measure RMSE after tasks 355 (larger knob faces), 356 (knob highlight brightness), and 357
(reduced grid line alpha). Establish new baselines for all sub-regions.

## Produces
None

## Consumes
None

## Relevant Files
Read: `Scripts/ui-test-helper.sh` — screenshot capture methodology
Read: `screenshots/task-354-rmse-results.txt` — previous baseline

## Acceptance Criteria
- [ ] Run full RMSE measurement using correct methodology → Expected: results captured
- [ ] Full-image RMSE reported ≤ 22.08% (task-354 baseline)
- [ ] All sub-region measurements recorded in `screenshots/task-358-rmse-results.txt`

## Tests
None

## Technical Details

**CORRECT methodology:**

```bash
# Step 1: Prepare reference
convert /reference-docs/reference-screenshots/prol2-main-ui.jpg \
    -crop 1712x1073+97+32 +repage -resize 900x500! /tmp/ref358.png

# Step 2: Rebuild and launch
cd /workspace/M-LIM
cmake --build build -j$(nproc)
source /workspace/Scripts/ui-test-helper.sh
start_app
sleep 3
scrot /tmp/task358-raw.png
stop_app

# Step 3: Crop to plugin area
convert /tmp/task358-raw.png -crop 908x500+509+325 +repage -resize 900x500! /tmp/task358-mlim.png

# Step 4: Full image RMSE
compare -metric RMSE /tmp/ref358.png /tmp/task358-mlim.png /tmp/task358-diff.png 2>&1

# Step 5: Sub-region RMSE
for region in "Waveform:600x400+150+50" "LeftMeters:30x378+0+30" "RightPanel:100x400+800+50" "ControlStrip:900x60+0+440"; do
  name="${region%%:*}"; crop="${region##*:}"
  rmse=$(compare -metric RMSE \
    <(convert /tmp/ref358.png -crop $crop +repage png:-) \
    <(convert /tmp/task358-mlim.png -crop $crop +repage png:-) \
    /dev/null 2>&1 | grep -Eo '0\.[0-9]+' | head -1)
  echo "$name: $rmse ($(echo "${rmse:-0} * 100" | bc -l | awk '{printf "%.2f%%", $1}'))"
done
```

**Baselines from task-354 (CORRECT methodology):**
- Full image:  22.08%
- Waveform:    20.55%
- Left Meters: 23.50%
- Right Panel: 29.59%
- Control Strip: 22.42% (note: fresh measurement on current build shows ~70% for this region;
  task-358 should report the actual current value, not the task-352 baseline)

Save full results to `screenshots/task-358-rmse-results.txt`.

## Dependencies
Requires tasks 355, 356, 357 (or whichever completed)
