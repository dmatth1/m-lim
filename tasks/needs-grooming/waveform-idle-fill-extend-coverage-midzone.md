# Task: Waveform Display — Brighter Idle Fill Color for Mid-Zone

## Description

The waveform idle fill (task 369) simulates active audio appearance using `inputWaveform`
color (0xCC6878A0 = R=104, G=120, B=160). However, the reference mid-zone (y=250 in 500px
crop) shows R=121, G=128, B=155 — which exceeds `inputWaveform.R` (104). This means
no alpha value can reach reference brightness in the R channel using the current color.

**Gap analysis:**
- Reference at mid-zone (y=250): R=121, G=128, B=155
- M-LIM current at mid-zone:     R=75,  G=84,  B=117  (gap: +46R, +44G, +38B)
- At full alpha=1.0 with inputWaveform: R=104 — still 17 units short in R

**Root cause:** The idle fill color (inputWaveform = R=104, G=120, B=160) is too blue-shifted
and not bright enough in the R and G channels to simulate the reference's warm bright waveform.
The reference shows light steel-blue content (R>120) while our fill color maxes at R=104.

**Fix:** Add a second, brighter overlay color for the idle simulation in `WaveformDisplay.cpp`.
Use a custom brighter idle fill color (R=130, G=138, B=165 ≈ 0xff828AA5) at the mid-zone:

```cpp
// Existing fill 1: lower 56% coverage (unchanged)
{
    const float fillFrac = 0.56f;
    ...inputWaveform.withAlpha (0.62f)...
}

// New fill 2: mid-zone boost for upper-middle brightness
// Covers 28%–60% of display height (the zone where reference is brightest)
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

The midFillColour brightness (0xff828AA5) is chosen to bring the composite closer to
reference at 40-45% alpha: R = 0.42*130 + 0.58*75 = 55+44 = 99, close to ref 121.
Combined with the existing fill, target composite: ~100-108 in R.

**Alternative values to test:** 0xff909AAF (R=144, G=154, B=175) at alpha 0.35 if
the above is insufficient; reduce alpha if over-bright.

**Expected improvement:** ~0.5–1.0 pp in wave region RMSE.

**Risk assessment:** Medium. Added second gradient pass over mid-zone. Test with RMSE
measurement before committing. If full RMSE regresses, reduce alpha or narrow the range.

## Produces
None

## Consumes
None

## Relevant Files
Modify: `M-LIM/src/ui/WaveformDisplay.cpp` — `drawBackground()` method, after the existing
        idle fill block (lines 292–305)

## Acceptance Criteria
- [ ] Run: wave region RMSE → Expected: < 19.44% (current baseline)
- [ ] Run: full image RMSE → Expected: ≤ 21.22% (no regression)

## Tests
None

## Technical Details

The idle fill block to modify is in `WaveformDisplay::drawBackground()`:
```cpp
// Lines 292–305: existing idle fill block
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

Add the second fill block immediately after. Keep the existing first fill unchanged.

Measurement:
```bash
# Build
export CCACHE_DIR=/build-cache && cmake --build /workspace/M-LIM/build --target MLIM_Standalone -j$(nproc)

convert /reference-docs/reference-screenshots/prol2-main-ui.jpg \
    -crop 1712x1073+97+32 +repage -resize 900x500! /tmp/ref.png

pkill -f "M-LIM" 2>/dev/null; sleep 1
DISPLAY=:99 /workspace/M-LIM/build/MLIM_artefacts/Release/Standalone/M-LIM &>/dev/null &
sleep 8; DISPLAY=:99 scrot /tmp/raw.png; pkill -f "M-LIM"
convert /tmp/raw.png -crop 908x500+509+325 +repage -resize 900x500! /tmp/mlim.png

# Wave RMSE
convert /tmp/mlim.png -crop 640x500+0+0 +repage /tmp/wave-mlim.png
convert /tmp/ref.png  -crop 640x500+0+0 +repage /tmp/wave-ref.png
compare -metric RMSE /tmp/wave-ref.png /tmp/wave-mlim.png /dev/null 2>&1

# Full RMSE (check no regression)
compare -metric RMSE /tmp/ref.png /tmp/mlim.png /dev/null 2>&1
```

## Dependencies
None
