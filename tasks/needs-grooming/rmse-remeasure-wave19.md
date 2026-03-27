# Task: RMSE Remeasure — Wave 19 Baseline

## Description

After completing all wave-19 visual parity tasks (waveform mid-zone brightness boost,
top gradient warming, bottom gradient brightening, control strip brightening, LUFS
histogram brightening), remeasure the full RMSE baseline.

This task establishes the wave-19 RMSE baseline and records it in the harness state.

## Produces
None

## Consumes
None

## Relevant Files
Read: `screenshots/` — prior RMSE results for comparison

## Acceptance Criteria
- [ ] Run: full build + screenshot pipeline → Expected: RMSE measured and recorded
- [ ] Run: harness-state.json RMSE fields updated → Expected: wave-19 baseline committed

## Tests
None

## Technical Details

```bash
# Build
cmake --build /tmp/mlim-build --target MLIM_Standalone -j$(nproc)

# Reference crop
convert /reference-docs/reference-screenshots/prol2-main-ui.jpg \
    -crop 1712x1073+97+32 +repage -resize 900x500! /tmp/ref.png

# Capture
pkill -f "Standalone/M-LIM" 2>/dev/null; sleep 1
DISPLAY=:44 /tmp/mlim-build/MLIM_artefacts/Release/Standalone/M-LIM &>/dev/null &
sleep 6
DISPLAY=:44 scrot /tmp/raw.png
pkill -f "Standalone/M-LIM"
convert /tmp/raw.png -crop 908x500+509+325 +repage -resize 900x500! /tmp/mlim.png

# Full RMSE
compare -metric RMSE /tmp/ref.png /tmp/mlim.png /dev/null 2>&1

# Wave region (left 70%)
convert /tmp/mlim.png -crop 630x500+0+0 /tmp/wave-cur.png
convert /tmp/ref.png  -crop 630x500+0+0 /tmp/wave-ref.png
compare -metric RMSE /tmp/wave-ref.png /tmp/wave-cur.png /dev/null 2>&1

# Right panel
convert /tmp/mlim.png -crop 270x430+660+0 /tmp/right-cur.png
convert /tmp/ref.png  -crop 270x430+660+0 /tmp/right-ref.png
compare -metric RMSE /tmp/right-ref.png /tmp/right-cur.png /dev/null 2>&1
```

Record results in `screenshots/task-NNN-rmse-results.txt` and commit.

## Dependencies
Requires all wave-19 visual tasks to be complete before running.
