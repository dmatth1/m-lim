# Task: Waveform Mid-Zone Idle Fill — Boost Opacity and Extend Coverage

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
1. Increase mid-fill max alpha from `0.42f` to `0.68f`.
2. Extend mid-fill bottom from 60% to 75% of waveform height so the fill reaches
   the y=280–330 zone where the gap is largest.
3. Shift the gradient so the peak alpha is at 50% (current peak is at 28% top).

New gradient specification:
```cpp
// Mid-zone boost (covers 28%–75% from top, peak alpha at 50%)
const float midTop  = area.getY() + area.getHeight() * 0.28f;
const float midPeak = area.getY() + area.getHeight() * 0.50f;
const float midBot  = area.getY() + area.getHeight() * 0.75f;
juce::Colour midFillColour { 0xff828AA5 };
juce::ColourGradient midFillGrad (
    midFillColour.withAlpha (0.0f),   0.0f, midTop,
    midFillColour.withAlpha (0.68f),  0.0f, midPeak,
    false);
midFillGrad.addColour (1.0f, midFillColour.withAlpha (0.0f));
// NOTE: juce doesn't support 3-stop gradient in the constructor; use addColour:
```

Actually implement as two overlapping gradients or adjust the existing one:
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

Replace the existing single `midFillGrad` block with the two-pass approach above.

**Expected improvement:** 0.8–1.5 pp in wave RMSE (biggest remaining lever).

## Produces
None

## Consumes
None

## Relevant Files
Modify: `src/ui/WaveformDisplay.cpp` — `drawBackground()` method, mid-zone fill block (around line 307–317)
Read: `src/ui/Colours.h` — `inputWaveform`, `displayGradientTop/Bottom`

## Acceptance Criteria
- [ ] Run: build + screenshot + wave RMSE (crop 640x500+0+0 of the 900x500 comparison) → Expected: wave RMSE decreases below current 18.96%
- [ ] Run: full image RMSE → Expected: full RMSE stays ≤ 21.5% (no regression)
- [ ] Run: visual check → Expected: waveform mid-zone shows brighter steel-blue fill centered around 50% height; top 28% and bottom 75%+ unchanged

## Tests
None

## Technical Details
Build command: `cmake --build /tmp/mlim-build --target MLIM_Standalone -j$(nproc)`

RMSE measurement:
```bash
convert /reference-docs/reference-screenshots/prol2-main-ui.jpg \
    -crop 1712x1073+97+32 +repage -resize 900x500! /tmp/ref.png

pkill -f "Standalone/M-LIM" 2>/dev/null; sleep 1
DISPLAY=:44 /tmp/mlim-build/MLIM_artefacts/Release/Standalone/M-LIM &>/dev/null &
sleep 6
DISPLAY=:44 scrot /tmp/raw.png
pkill -f "Standalone/M-LIM"

convert /tmp/raw.png -crop 908x500+509+325 +repage -resize 900x500! /tmp/mlim.png
compare -metric RMSE /tmp/ref.png /tmp/mlim.png /dev/null 2>&1
convert /tmp/mlim.png -crop 640x500+0+0 /tmp/wave.png
convert /tmp/ref.png  -crop 640x500+0+0 /tmp/wref.png
compare -metric RMSE /tmp/wref.png /tmp/wave.png /dev/null 2>&1
```

## Dependencies
None
