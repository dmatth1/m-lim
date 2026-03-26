# Task 308: RMSE Remeasure After Tasks 304–309

## Description
Measure the RMSE improvement after visual tasks 304–307 are merged. This provides a quantitative
checkpoint before the next audit cycle.

**Baseline (task 303):**
- Full image: 24.19%
- Left level meters (30x378 at x=0, y=30): 26.00%
- Waveform area (600x400 at x=150, y=50): 23.03%
- Right panel (100x400 at x=800, y=50): 29.60%
- Control strip (900x60 at x=0, y=440): 22.56%

**Target:** Full image < 22.0% (2.19pp improvement from tasks 304–309)

## Produces
None

## Consumes
artifact:build/MLIM_artefacts/Release/Standalone/M-LIM

## Relevant Files
Read: `M-LIM/Scripts/ui-test-helper.sh` — if present; otherwise use manual method below
Read: `screenshots/task-299-rmse-results.txt` — reference for measurement methodology

## Acceptance Criteria
- [ ] Run: `cmake --build /workspace/M-LIM/build --config Release -j$(nproc) --target MLIM_Standalone 2>&1 | tail -3` → Expected: exit 0
- [ ] Run full RMSE comparison → Expected: full image RMSE < 0.230 (improvement from 0.2419)
- [ ] Save results to `screenshots/task-308-rmse-results.txt`

## Tests
None

## Technical Details
Use the same methodology as task 299:

```bash
# 1. Start Xvfb and app
pkill -9 Xvfb 2>/dev/null; sleep 1
Xvfb :99 -screen 0 1920x1080x24 -ac &
sleep 3
DISPLAY=:99 /workspace/M-LIM/build/MLIM_artefacts/Release/Standalone/M-LIM &
sleep 5

# 2. Capture screenshot
DISPLAY=:99 scrot /workspace/screenshots/task-308-raw.png

# 3. Crop plugin area (same bounds as task 299: 908x500 at 509,325)
convert screenshots/task-308-raw.png -crop 908x500+509+325 +repage \
    -resize 900x500! screenshots/task-308-mlim-cropped.png

# 4. Crop reference (same bounds as task 299)
convert /reference-docs/reference-screenshots/prol2-main-ui.jpg \
    -crop 1712x1073+97+32 +repage -resize 900x500! screenshots/task-308-ref-cropped.png

# 5. Full image RMSE
compare -metric RMSE screenshots/task-308-ref-cropped.png \
    screenshots/task-308-mlim-cropped.png screenshots/task-308-diff-full.png 2>&1

# 6. Sub-region RMSE (same regions as previous)
# Left meters: 30x378 at x=0, y=30
# Waveform: 600x400 at x=150, y=50
# Right panel: 100x400 at x=800, y=50
# Control strip: 900x60 at x=0, y=440
```

Save a results summary to `screenshots/task-308-rmse-results.txt` with:
- Full image RMSE and percentage
- Each sub-region RMSE and percentage
- Change vs task-303 baseline for each

## Dependencies
Requires tasks 304, 305, 306, 307, 309
