# Task 391: Waveform Top Gradient — Warm Color Shift

## Description

Pixel analysis of the waveform top area (crop y=30–130) shows the reference has a warm
brownish-purple near-black tone while M-LIM renders a cooler blue-gray:

| Row | Reference  | M-LIM      | Issue         |
|-----|-----------|------------|---------------|
| y=30 | 42343B   | 2D2D2D     | Too cold/dark  |
| y=80 | 39332A   | 292733     | Too cool/blue  |
| y=130| 323440   | 2F3041     | Close          |

`displayGradientTop = 0xff242027` sets the near-black at the top of the waveform
(R=36, G=32, B=39 — slightly cool purple-gray).
The reference at y=80 shows `39332A` (R=57, G=51, B=42) — a warm brownish-black.

**Fix:** Change `displayGradientTop` in `Colours.h` to a warmer, slightly brighter value:
```cpp
const juce::Colour displayGradientTop { 0xff332A2D };  // warm near-black (was 0xff242027)
```

`0xff332A2D` = R:51, G:42, B:45 — shifts toward the warm reference `39332A` (R:57,G:51,B:42)
while staying dark enough to preserve the characteristic deep-top appearance.

If this causes wave RMSE regression, try `0xff2D2629` (more subtle shift).

**Expected improvement:** 0.2–0.5 pp in wave RMSE (top-area correction).

## Produces
None

## Consumes
None

## Relevant Files
Modify: `src/ui/Colours.h` — `displayGradientTop` color constant
Read: `src/ui/WaveformDisplay.cpp` — how `displayGradientTop` is used in `drawBackground()`

## Acceptance Criteria
- [ ] Run: build + screenshot + full RMSE → Expected: full RMSE ≤ 20.74% (no regression vs wave-18 baseline)
- [ ] Run: wave region RMSE (crop 640x500+0+0) → Expected: wave RMSE decreases from 18.96%
- [ ] Run: pixel sample at y=80 of comparison crop → Expected: closer to reference warm `39332A`

## Tests
None

## Technical Details

In `src/ui/Colours.h`, change:
```cpp
const juce::Colour displayGradientTop   { 0xff242027 };
```
to:
```cpp
const juce::Colour displayGradientTop   { 0xff332A2D };
```

Build and measure:
```bash
export CCACHE_DIR=/build-cache
cmake --build /workspace/M-LIM/build --target MLIM_Standalone_Standalone -j$(nproc)

convert /reference-docs/reference-screenshots/prol2-main-ui.jpg \
    -crop 1712x1073+97+32 +repage -resize 900x500! /tmp/ref391.png

pkill -f "Standalone/M-LIM" 2>/dev/null; sleep 1
DISPLAY=:99 /workspace/M-LIM/build/MLIM_artefacts/Release/Standalone/M-LIM &>/dev/null &
sleep 8; DISPLAY=:99 scrot /tmp/raw391.png; pkill -f "Standalone/M-LIM"
convert /tmp/raw391.png -crop 908x500+509+325 +repage -resize 900x500! /tmp/mlim391.png

echo "=== Full RMSE ==="
compare -metric RMSE /tmp/ref391.png /tmp/mlim391.png /dev/null 2>&1
convert /tmp/mlim391.png -crop 640x500+0+0 +repage /tmp/wave391.png
convert /tmp/ref391.png  -crop 640x500+0+0 +repage /tmp/wref391.png
echo "=== Wave RMSE ==="
compare -metric RMSE /tmp/wref391.png /tmp/wave391.png /dev/null 2>&1

# Pixel warmth check at y=80
convert /tmp/mlim391.png -crop 400x1+100+80 -scale 1x1! -format '%[hex:u]' info:
# Should be closer to 39332A than current 292733
```

Save results to `screenshots/task-391-rmse-results.txt` and crop to `screenshots/task-391-crop.png`.
Commit: `worker-N: complete task 391 - waveform top warm shift (wave X%, full Y%)`

## Dependencies
None
