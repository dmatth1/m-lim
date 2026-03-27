# Task 389: Waveform Display Gradient — Brighten Bottom Color

## Description

The waveform display bottom half (y=330–380 in 900x500 crop) is ~20–30 units too dark
compared to the reference:

| Row | Reference  | M-LIM      | Gap   |
|-----|-----------|------------|-------|
| y=330| 8C92AE  | 6E799A     | ~30 u |
| y=380| 7B819B  | 5F6F98     | ~24 u |

`displayGradientBottom = 0xff506090` = R:80, G:96, B:144.
The reference at y=330 shows `8C92AE` (R:140, G:146, B:174).

Increasing the gradient bottom color will lift the entire lower half of the display.

**Fix:**
```cpp
const juce::Colour displayGradientBottom { 0xff606898 };  // brightened from 506090
```

`0xff606898` = R:96, G:104, B:152 — adds +16 to R, +8 to G, +8 to B,
preserving the blue-dominant character while lifting overall brightness.

If this causes RMSE regression in the bottom zone (y=420 is already close at current),
try `0xff587090` (+8 in R only) as a more conservative alternative.

**Expected improvement:** 0.3–0.6 pp in wave RMSE.

## Produces
None

## Consumes
None

## Relevant Files
Modify: `src/ui/Colours.h` — `displayGradientBottom` constant
Read: `src/ui/WaveformDisplay.cpp` — `drawBackground()` uses this for the background gradient

## Acceptance Criteria
- [ ] Run: build + screenshot + wave RMSE (crop 640x500+0+0) → Expected: wave RMSE decreases from 18.96%
- [ ] Run: full RMSE → Expected: ≤ 20.74% (wave-18 baseline, no regression)
- [ ] Run: pixel at y=330 in comparison crop → Expected: closer to `8C92AE` than current `6E799A`

## Tests
None

## Technical Details

In `src/ui/Colours.h`, change:
```cpp
const juce::Colour displayGradientBottom{ 0xff506090 };
```
to:
```cpp
const juce::Colour displayGradientBottom{ 0xff606898 };
```

Build and measure:
```bash
export CCACHE_DIR=/build-cache
cmake --build /workspace/M-LIM/build --target MLIM_Standalone_Standalone -j$(nproc)

convert /reference-docs/reference-screenshots/prol2-main-ui.jpg \
    -crop 1712x1073+97+32 +repage -resize 900x500! /tmp/ref389.png

pkill -f "Standalone/M-LIM" 2>/dev/null; sleep 1
DISPLAY=:99 /workspace/M-LIM/build/MLIM_artefacts/Release/Standalone/M-LIM &>/dev/null &
sleep 8; DISPLAY=:99 scrot /tmp/raw389.png; pkill -f "Standalone/M-LIM"
convert /tmp/raw389.png -crop 908x500+509+325 +repage -resize 900x500! /tmp/mlim389.png

echo "=== Full RMSE ==="
compare -metric RMSE /tmp/ref389.png /tmp/mlim389.png /dev/null 2>&1
convert /tmp/mlim389.png -crop 640x500+0+0 +repage /tmp/wave389.png
convert /tmp/ref389.png  -crop 640x500+0+0 +repage /tmp/wref389.png
echo "=== Wave RMSE ==="
compare -metric RMSE /tmp/wref389.png /tmp/wave389.png /dev/null 2>&1

# Bottom zone check (should not regress)
convert /tmp/mlim389.png -crop 400x1+100+330 -scale 1x1! -format '%[hex:u]' info:
# Should be closer to 8C92AE than current 6E799A
```

Save results to `screenshots/task-389-rmse-results.txt` and crop to `screenshots/task-389-crop.png`.
Commit: `worker-N: complete task 389 - waveform bottom gradient brightened (wave X%, full Y%)`

## Dependencies
None
