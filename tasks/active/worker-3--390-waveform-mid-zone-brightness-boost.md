# Task 390: Waveform Mid-Zone Idle Fill — Boost Opacity and Extend Coverage

## Description

Pixel analysis shows the waveform mid-zone (y=230–330 in the 900x500 comparison crop) is
40–53 units too dark versus the reference, making it the single largest contributor to wave
RMSE (wave region currently 18.96%).

Measured gap at key rows:
| Row (crop) | Reference  | M-LIM      | Gap    |
|------------|-----------|------------|--------|
| y=230      | 6F799B    | 464E69     | ~41 u  |
| y=280      | 8289A4    | 4D587C     | ~53 u  |
| y=330      | 8C92AE    | 6E799A     | ~30 u  |

The existing mid-zone fill (`midFillGrad` in `drawBackground()`) covers 28%–60% of
waveform height with max alpha 0.42. This fill barely reaches the y=280 region, and at
max alpha 0.42 the brightness contribution is too low.

**Fix:**
Replace the existing single `midFillGrad` block in `WaveformDisplay::drawBackground()`
with a two-pass approach that extends coverage and increases alpha:

```cpp
{
    const float midTop   = area.getY() + area.getHeight() * 0.28f;
    const float midMid   = area.getY() + area.getHeight() * 0.50f;
    const float midBot   = area.getY() + area.getHeight() * 0.75f;
    juce::Colour midFill { 0xff828AA5 };

    // Rising half: transparent at 28% → 0.68 at 50%
    juce::ColourGradient riseGrad (
        midFill.withAlpha (0.0f),   0.0f, midTop,
        midFill.withAlpha (0.68f),  0.0f, midMid,
        false);
    g.setGradientFill (riseGrad);
    g.fillRect (area.getX(), midTop, area.getWidth(), midMid - midTop);

    // Falling half: 0.68 at 50% → transparent at 75%
    juce::ColourGradient fallGrad (
        midFill.withAlpha (0.68f),  0.0f, midMid,
        midFill.withAlpha (0.0f),   0.0f, midBot,
        false);
    g.setGradientFill (fallGrad);
    g.fillRect (area.getX(), midMid, area.getWidth(), midBot - midMid);
}
```

**Expected improvement:** 0.8–1.5 pp in wave RMSE (biggest remaining lever).

## Produces
None

## Consumes
None

## Relevant Files
Modify: `src/ui/WaveformDisplay.cpp` — `drawBackground()` method, mid-zone fill block (around line 307–317)
Read: `src/ui/Colours.h` — `inputWaveform`, `displayGradientTop/Bottom` for context

## Acceptance Criteria
- [ ] Run: build + screenshot + wave RMSE (crop 640x500+0+0 of 900x500) → Expected: wave RMSE decreases below 18.96%
- [ ] Run: full image RMSE → Expected: ≤ 21.5% (no significant regression)
- [ ] Run: visual check → Expected: waveform mid-zone shows brighter steel-blue fill centered around 50% height; top 28% and bottom 75%+ unchanged

## Tests
None

## Technical Details

Locate the mid-zone fill block in `WaveformDisplay::drawBackground()`. It likely looks like:
```cpp
juce::ColourGradient midFillGrad (...);
midFillGrad.addColour (...);
g.setGradientFill (midFillGrad);
g.fillRect (...);
```

Replace the entire block with the two-pass implementation above.

Build and measure:
```bash
export CCACHE_DIR=/build-cache
cmake --build /workspace/M-LIM/build --target MLIM_Standalone_Standalone -j$(nproc)

convert /reference-docs/reference-screenshots/prol2-main-ui.jpg \
    -crop 1712x1073+97+32 +repage -resize 900x500! /tmp/ref390.png

pkill -f "Standalone/M-LIM" 2>/dev/null; sleep 1
DISPLAY=:99 /workspace/M-LIM/build/MLIM_artefacts/Release/Standalone/M-LIM &>/dev/null &
sleep 8; DISPLAY=:99 scrot /tmp/raw390.png; pkill -f "Standalone/M-LIM"
convert /tmp/raw390.png -crop 908x500+509+325 +repage -resize 900x500! /tmp/mlim390.png

echo "=== Full RMSE ==="
compare -metric RMSE /tmp/ref390.png /tmp/mlim390.png /dev/null 2>&1
convert /tmp/mlim390.png -crop 640x500+0+0 +repage /tmp/wave390.png
convert /tmp/ref390.png  -crop 640x500+0+0 +repage /tmp/wref390.png
echo "=== Wave RMSE ==="
compare -metric RMSE /tmp/wref390.png /tmp/wave390.png /dev/null 2>&1
```

Save results to `screenshots/task-390-rmse-results.txt` and crop to `screenshots/task-390-crop.png`.
Commit: `worker-N: complete task 390 - waveform mid-zone boost (wave X%, full Y%)`

## Dependencies
None
