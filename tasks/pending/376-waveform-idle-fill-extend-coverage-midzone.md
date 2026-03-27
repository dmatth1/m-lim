# Task 376: Waveform Display — Extend Idle Fill Coverage to Mid-Zone

## Description

The waveform idle fill (task 369) uses `inputWaveform` color (R=104, G=120, B=160) but
the reference mid-zone shows R=121, G=128, B=155 — exceeding `inputWaveform.R` (104) so
no alpha value alone can reach reference brightness in the R channel.

**Gap analysis:**
- Reference at mid-zone (y=250): R=121, G=128, B=155
- M-LIM current at mid-zone:     R=75,  G=84,  B=117  (gap: +46R, +44G, +38B)
- At full alpha=1.0 with inputWaveform: R=104 — still 17 units short in R

**Fix:** Add a second, brighter overlay gradient for the idle mid-zone. Use a custom
brighter idle fill color `0xff828AA5` (R=130, G=138, B=165) covering 28%–60% of height:

```cpp
// New fill 2: mid-zone boost (covers 28%–60% from top)
{
    const float midTop  = area.getY() + area.getHeight() * 0.28f;
    const float midBot  = area.getY() + area.getHeight() * 0.60f;
    juce::Colour midFillColour { 0xff828AA5 };
    juce::ColourGradient midFillGrad (
        midFillColour.withAlpha (0.42f), 0.0f, midTop,
        midFillColour.withAlpha (0.0f),  0.0f, midBot,
        false);
    g.setGradientFill (midFillGrad);
    g.fillRect (area.getX(), midTop, area.getWidth(), midBot - midTop);
}
```

Expected composite at mid-zone: R ≈ 100-108 (vs reference 121, improved from 75).

**Expected improvement:** ~0.5–1.0 pp in wave region RMSE.

**Note:** This only modifies `WaveformDisplay.cpp`. Task 374 and 375 modify `Colours.h`
and are independent; these three can run in parallel.

## Produces
None

## Consumes
None

## Relevant Files
Modify: `src/ui/WaveformDisplay.cpp` — `drawBackground()` method, add mid-zone fill block after existing idle fill
Read: `src/ui/Colours.h` — `inputWaveform` color constant
Skip: `src/ui/LoudnessPanel.cpp` — unrelated

## Acceptance Criteria
- [ ] Run: build + screenshot + wave region RMSE → Expected: < 19.44% (wave 16 baseline)
- [ ] Run: full image RMSE → Expected: ≤ 21.22% (no regression)
- [ ] Run: visual check → Expected: waveform mid-zone shows subtle bright steel-blue fill, no harsh banding

## Tests
None

## Technical Details

In `WaveformDisplay::drawBackground()`, locate the existing idle fill block (~lines 292–305):
```cpp
{
    const float fillFrac = 0.56f;
    const float fillTop  = area.getBottom() - area.getHeight() * fillFrac;
    juce::ColourGradient fillGrad (
        MLIMColours::inputWaveform.withAlpha (0.0f),   0.0f, fillTop,
        MLIMColours::inputWaveform.withAlpha (0.62f),  0.0f, area.getBottom(),
        false);
    g.setGradientFill (fillGrad);
    g.fillRect (area.getX(), fillTop, area.getWidth(), area.getBottom() - fillTop);
}
```

Add the second fill block immediately after (keep first fill unchanged):
```cpp
// Mid-zone brightness boost for idle simulation
{
    const float midTop  = area.getY() + area.getHeight() * 0.28f;
    const float midBot  = area.getY() + area.getHeight() * 0.60f;
    juce::Colour midFillColour { 0xff828AA5 };  // bright steel-blue, R=130, G=138, B=165
    juce::ColourGradient midFillGrad (
        midFillColour.withAlpha (0.42f), 0.0f, midTop,
        midFillColour.withAlpha (0.0f),  0.0f, midBot,
        false);
    g.setGradientFill (midFillGrad);
    g.fillRect (area.getX(), midTop, area.getWidth(), midBot - midTop);
}
```

If RMSE worsens, try `0xff909AAF` at alpha 0.35, or reduce alpha to 0.30.

Build and measure:
```bash
export CCACHE_DIR=/build-cache
cmake --build /workspace/M-LIM/build --target MLIM_Standalone_Standalone -j$(nproc)

convert /reference-docs/reference-screenshots/prol2-main-ui.jpg \
    -crop 1712x1073+97+32 +repage -resize 900x500! /tmp/ref.png

pkill -f "Standalone/M-LIM" 2>/dev/null; sleep 1
DISPLAY=:99 /workspace/M-LIM/build/MLIM_artefacts/Release/Standalone/M-LIM &>/dev/null &
sleep 8; DISPLAY=:99 scrot /tmp/raw.png; pkill -f "Standalone/M-LIM"
convert /tmp/raw.png -crop 908x500+509+325 +repage -resize 900x500! /tmp/mlim.png

# Wave RMSE
convert /tmp/mlim.png -crop 640x500+0+0 +repage /tmp/wave-mlim.png
convert /tmp/ref.png  -crop 640x500+0+0 +repage /tmp/wave-ref.png
compare -metric RMSE /tmp/wave-ref.png /tmp/wave-mlim.png /dev/null 2>&1

# Full RMSE
compare -metric RMSE /tmp/ref.png /tmp/mlim.png /dev/null 2>&1
```

## Dependencies
None
