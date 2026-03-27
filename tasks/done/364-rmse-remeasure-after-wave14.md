# Task 364: RMSE Remeasure After Wave 14 Changes

## Description
Measure RMSE after wave 14 tasks (361: grid alpha revert, 362: output meter improvement,
363: left meter edge gradient). Establish new baselines for all sub-regions and validate
improvements against wave 13 state.

## Produces
None

## Consumes
None

## Relevant Files
Read: `screenshots/task-358-rmse-results.txt` — wave 13 baseline
Read: `Scripts/ui-test-helper.sh` — screenshot capture methodology

## Acceptance Criteria
- [ ] Run full RMSE measurement → Expected: results captured in `screenshots/task-364-rmse-results.txt`
- [ ] Waveform RMSE reported → Expected: ≤21.00% (improvement from 21.87% wave 13 regression)
- [ ] Right panel RMSE reported → Expected: ≤28.50% (improvement from 29.59%)
- [ ] Full image RMSE reported → Expected: ≤22.33% (at or below wave 13 full)

## Tests
None

## Technical Details

**CORRECT measurement methodology:**

```bash
# Step 1: Prepare reference
convert /reference-docs/reference-screenshots/prol2-main-ui.jpg \
    -crop 1712x1073+97+32 +repage -resize 900x500! /tmp/ref364.png

# Step 2: Build
export CCACHE_DIR=/build-cache
cmake -B /tmp/build364 -S M-LIM -DCMAKE_BUILD_TYPE=Release -DCMAKE_CXX_COMPILER_LAUNCHER=ccache
cmake --build /tmp/build364 -j$(nproc)

# Step 3: Launch and capture
source Scripts/ui-test-helper.sh
start_app
sleep 3
scrot /tmp/task364-raw.png
stop_app

# Step 4: Crop to plugin area
convert /tmp/task364-raw.png -crop 908x500+509+325 +repage -resize 900x500! /tmp/task364-mlim.png

# Step 5: Full image RMSE
echo "=== Full image RMSE ===" && \
compare -metric RMSE /tmp/ref364.png /tmp/task364-mlim.png /tmp/task364-diff.png 2>&1

# Step 6: Sub-region RMSE
for region in "Waveform:600x400+150+50" "LeftMeters:30x378+0+30" "RightPanel:100x400+800+50" "ControlStrip:900x60+0+440"; do
  name="${region%%:*}"; crop="${region##*:}"
  echo "=== $name ($crop) ==="
  compare -metric RMSE \
    <(convert /tmp/ref364.png -crop $crop +repage png:-) \
    <(convert /tmp/task364-mlim.png -crop $crop +repage png:-) \
    /dev/null 2>&1
done
```

**Wave 13 baseline (task-358 remeasure):**
- Full image:  22.33%
- Waveform:    21.87% (REGRESSED — target: recover to ≤21.00%)
- Left Meters: 23.50%
- Right Panel: 29.59% (NOT MET — target: ≤27.00%)
- Control Strip: 20.74%

Save all results to `screenshots/task-364-rmse-results.txt` with comparison to wave 13 baseline.

## Dependencies
Requires tasks 361, 362, 363
