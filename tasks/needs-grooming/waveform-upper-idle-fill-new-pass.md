# Task: Waveform Upper Idle Fill — New Brightness Pass (y=15%–50%)

## Description

Pixel analysis reveals M-LIM's waveform display is significantly too dark in the upper-mid zone
(y=150 in the 500px image = approximately 30% from the top of the waveform area). This zone falls
ABOVE the existing lower idle fill (which starts at 44% from top) and ABOVE the midzone tent
gradients (which start at 36%).

**Measured gap at y=150, x=300:**
- M-LIM: srgb(62, 67, 84)
- Reference: srgb(108, 123, 168)
- Delta: +46R, +56G, +84B — M-LIM is ~60 units too dark

The reference at this position shows the dense input waveform fill (blue-gray composite) that
accumulates at the typical listening level (-6 to -12 dBFS = 20–40% height in the waveform display).
M-LIM has no idle fill in this zone at all — the current idle fill starts at fillFrac=0.56 (44% from top).

**Fix**: Add a new upward-facing idle fill pass in `WaveformDisplay::drawBackground()`. This pass
should target the y=15%–50% range, centered around y=25-30%, using the inputWaveform color
at moderate alpha (0.55–0.70 peak). This approximates the dense waveform content that is
visible in the reference at this height.

## Relevant Files

Modify: `src/ui/WaveformDisplay.cpp` — add new gradient pass in `drawBackground()` after the
  existing gradient fill and before the midzone tents (around line 307)
Read: `src/ui/Colours.h` — inputWaveform constant (#6878A0)
Read: `screenshots/task-397-rmse-results.txt` — wave-21 baseline to compare against

## Acceptance Criteria

- [ ] Run RMSE on wave region after change → Expected: wave region RMSE ≤ 16.72% (wave-21 baseline)
- [ ] Run RMSE on full image → Expected: full RMSE ≤ 19.46% (wave-21 baseline)
- [ ] Save screenshot crop to `screenshots/task-NNN-after.png`
- [ ] Save RMSE results to `screenshots/task-NNN-rmse-results.txt`

## Tests
None

## Technical Details

**Measured target**:
- At y=150 (30% height): target srgb(108,123,168) vs current srgb(62,67,84)
- At y=50 (top area): must remain close to srgb(38,33,39) — do NOT over-brighten the top

**Implementation approach** — add this block after the base gradient fill (~line 291):
```cpp
// Upper idle fill — approximates dense input waveform content in -6 to -12 dBFS zone
// Covers y=15%–55% height (centered around 28–32%), peak alpha ~0.60
// Addresses 46-60 unit brightness gap observed at y=30% height vs reference
{
    const float uTop = area.getY() + area.getHeight() * 0.15f;
    const float uMid = area.getY() + area.getHeight() * 0.30f;
    const float uBot = area.getY() + area.getHeight() * 0.55f;
    juce::Colour uFill = MLIMColours::inputWaveform.withAlpha (1.0f);  // #6878A0

    // Rising: 0.0 at uTop → peak at uMid
    juce::ColourGradient rGrad (
        uFill.withAlpha (0.0f),   0.0f, uTop,
        uFill.withAlpha (0.60f),  0.0f, uMid,
        false);
    g.setGradientFill (rGrad);
    g.fillRect (area.getX(), uTop, area.getWidth(), uMid - uTop);

    // Falling: peak at uMid → 0.0 at uBot
    juce::ColourGradient fGrad (
        uFill.withAlpha (0.60f),  0.0f, uMid,
        uFill.withAlpha (0.0f),   0.0f, uBot,
        false);
    g.setGradientFill (fGrad);
    g.fillRect (area.getX(), uMid, area.getWidth(), uBot - uMid);
}
```

**Tuning guidance**:
- Start with peak alpha 0.60 and measure wave RMSE
- Try 0.50, 0.65, 0.70 and pick the best RMSE result
- If the fill is too blue (increases blue RMSE), try using a warmer fill color like `juce::Colour{0xff707498}` instead
- Do NOT let the fill extend above y=10% or it will over-brighten the top area that currently matches well

**RMSE methodology** (from task-397):
```bash
# Reference
convert /reference-docs/reference-screenshots/prol2-main-ui.jpg \
    -crop 1712x1073+97+32 +repage -resize 900x500! /tmp/ref.png

# M-LIM
DISPLAY=:99 /workspace/M-LIM/build/MLIM_artefacts/Release/Standalone/M-LIM &
sleep 8
DISPLAY=:99 scrot /tmp/raw.png
pkill -f "M-LIM"
convert /tmp/raw.png -crop 908x500+509+325 +repage -resize 900x500! /tmp/mlim.png

# Wave region
convert /tmp/mlim.png -crop 640x500+0+0 +repage /tmp/mlim-wave.png
convert /tmp/ref.png  -crop 640x500+0+0 +repage /tmp/ref-wave.png
compare -metric RMSE /tmp/mlim-wave.png /tmp/ref-wave.png /dev/null 2>&1

# Full RMSE
compare -metric RMSE /tmp/mlim.png /tmp/ref.png /dev/null 2>&1
```

## Dependencies
None
