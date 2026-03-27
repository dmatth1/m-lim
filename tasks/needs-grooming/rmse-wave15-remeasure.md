# Task: RMSE Remeasure — Wave 15 (Output Meter Warmth + LUFS Histogram Gradient)

## Description
Measure RMSE after wave 15 tasks:
- Output meter track background warmth fix (barTrackBackground + LevelMeter idle gradient alpha)
- LUFS histogram area waveform gradient background

Establish new baselines for all sub-regions and validate improvements against wave 14 state.

## Produces
None

## Consumes
None

## Relevant Files
Read: `screenshots/task-364-rmse-results.txt` — wave 14 baseline
Read: `Scripts/ui-test-helper.sh` — screenshot capture methodology

## Acceptance Criteria
- [ ] Run full RMSE measurement → Expected: results saved to `screenshots/task-NNN-rmse-results.txt`
- [ ] Right panel RMSE reported → Expected: ≤ 28.00% (improvement from 29.26% wave 14)
- [ ] Waveform RMSE reported → Expected: ≤ 20.50% (improvement from 21.45% wave 14)
- [ ] Full image RMSE reported → Expected: ≤ 21.50% (improvement from 22.25% wave 14)

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
cmake -B /tmp/buildW15 -S M-LIM -DCMAKE_BUILD_TYPE=Release \
    -DCMAKE_CXX_COMPILER_LAUNCHER=ccache \
    -DCCACHE_SLOPPINESS=pch_defines,time_macros,include_file_mtime,include_file_ctime
cmake --build /tmp/buildW15 --target MLIM_Standalone -j$(nproc)

# Step 3: Launch and capture
Xvfb :99 -screen 0 1920x1080x24 &
sleep 2
export DISPLAY=:99
/tmp/buildW15/MLIM_artefacts/Release/Standalone/M-LIM &
APP_PID=$!
sleep 4
scrot /tmp/taskW15-raw.png
kill $APP_PID

# Step 4: Crop to plugin area
convert /tmp/taskW15-raw.png -crop 908x500+509+325 +repage -resize 900x500! /tmp/taskW15-mlim.png

# Step 5: Full image RMSE
echo "=== Full image RMSE ===" && \
compare -metric RMSE /tmp/refW15.png /tmp/taskW15-mlim.png /tmp/taskW15-diff.png 2>&1

# Step 6: Sub-region RMSE
for region in "Waveform:600x400+150+50" "LeftMeters:30x378+0+30" "RightPanel:100x400+800+50" "ControlStrip:900x60+0+440"; do
  name="${region%%:*}"; crop="${region##*:}"
  echo "=== $name ($crop) ==="
  compare -metric RMSE \
    <(convert /tmp/refW15.png -crop $crop +repage png:-) \
    <(convert /tmp/taskW15-mlim.png -crop $crop +repage png:-) \
    /dev/null 2>&1
done
```

**Wave 14 baseline (task-364):**
- Full image:    22.25%
- Waveform:      21.45%
- Left Meters:   20.85%
- Right Panel:   29.26%  ← biggest gap; target ≤27% ongoing
- Control Strip: 20.74%

Save results to `screenshots/task-NNN-rmse-results.txt` with comparison to wave 14.

## Dependencies
Requires wave 15 tasks: output-meter-warm-background, lufs-histogram-waveform-gradient-background
