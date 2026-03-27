# Task 392: Control Strip Gradient — Additional +13 Brightening

## Description

Task 387 applied a +5 brightness increase to `controlStripTop/Bottom`. After that change,
a ~13-unit gap remains vs the reference:

| Area       | After task 387     | Reference  | Remaining gap |
|------------|--------------------|------------|---------------|
| Strip mid  | ~555462            | 686C80     | ~13–19 u      |

Row-by-row pixel analysis (post task 375 baseline) shows the control strip is
consistently darker than reference:

| Row | Reference  | M-LIM (pre-387) | Gap   |
|-----|-----------|-----------------|-------|
| y=432| 5F5E6C  | 4A4857          | ~21 u |
| y=445| 686C80  | 555462          | ~19 u |
| y=460| 5C6075  | 484554          | ~24 u |

After task 387 (+5 units), the remaining gap is ~13–18 units.

**Fix:** Further increase both strip colors by ~13 units from task 387's post-387 values:
```cpp
// After task 387 (do NOT apply if 387 hasn't run — check current Colours.h first):
// controlStripTop    = 0xff5C5870  (post-387)
// controlStripBottom = 0xff4A4858  (post-387)

// Target for this task:
const juce::Colour controlStripTop    { 0xff696578 };  // +13 from post-387 value
const juce::Colour controlStripBottom { 0xff565362 };  // +12 from post-387 value
```

**IMPORTANT:** Before changing values, read `src/ui/Colours.h` to confirm the current
values reflect task 387's changes. If `controlStripTop` is still `0xff575468` (pre-387),
apply a combined +18 increment directly instead of +13.

**Expected improvement:** 0.3–0.5 pp in control strip RMSE, ~0.2 pp full RMSE.

## Produces
None

## Consumes
None

## Relevant Files
Modify: `src/ui/Colours.h` — `controlStripTop` and `controlStripBottom` constants
Read: `src/ui/ControlStrip.cpp` — `paint()` method uses these for gradient fill

## Acceptance Criteria
- [ ] Run: build + screenshot + full RMSE → Expected: ≤ 20.74% (no regression vs wave-18)
- [ ] Run: control strip pixel sample → Expected: average closer to `686C80` than before
- [ ] Run: visual check → Expected: control strip visibly brighter/lighter without looking washed out

## Tests
None

## Technical Details

Read `src/ui/Colours.h` first to get the current controlStripTop/Bottom values. Then
increment toward the target `0xff696578` / `0xff565362`.

Build and measure:
```bash
export CCACHE_DIR=/build-cache
cmake --build /workspace/M-LIM/build --target MLIM_Standalone_Standalone -j$(nproc)

convert /reference-docs/reference-screenshots/prol2-main-ui.jpg \
    -crop 1712x1073+97+32 +repage -resize 900x500! /tmp/ref392.png

pkill -f "Standalone/M-LIM" 2>/dev/null; sleep 1
DISPLAY=:99 /workspace/M-LIM/build/MLIM_artefacts/Release/Standalone/M-LIM &>/dev/null &
sleep 8; DISPLAY=:99 scrot /tmp/raw392.png; pkill -f "Standalone/M-LIM"
convert /tmp/raw392.png -crop 908x500+509+325 +repage -resize 900x500! /tmp/mlim392.png

echo "=== Full RMSE ==="
compare -metric RMSE /tmp/ref392.png /tmp/mlim392.png /dev/null 2>&1
convert /tmp/mlim392.png -crop 900x90+0+410 +repage /tmp/ctrl392.png
convert /tmp/ref392.png  -crop 900x90+0+410 +repage /tmp/cref392.png
echo "=== Control strip RMSE ==="
compare -metric RMSE /tmp/cref392.png /tmp/ctrl392.png /dev/null 2>&1

# Pixel check at y=445
convert /tmp/mlim392.png -crop 400x1+100+445 -scale 1x1! -format '%[hex:u]' info:
# Should be closer to 686C80
```

Save results to `screenshots/task-392-rmse-results.txt` and crop to `screenshots/task-392-crop.png`.
Commit: `worker-N: complete task 392 - control strip brightened (ctrl X%, full Y%)`

## Dependencies
Requires task 387
