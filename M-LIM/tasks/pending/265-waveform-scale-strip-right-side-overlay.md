# Task 265: WaveformDisplay — Move Scale Labels to Right Side Overlaid on Waveform

## Description
The current WaveformDisplay reserves a `kScaleWidth` (~40 px) strip on the **LEFT** side of
the waveform area, fills it with `MLIMColours::background` (0xff1A1A1A — very dark), and draws
dB scale labels there. This produces a conspicuous dark vertical band on the left edge that is
completely absent from the FabFilter Pro-L 2 reference.

In the reference, the dB scale labels appear on the **RIGHT** edge of the waveform display area,
drawn directly overlaid on the waveform background (no separate darker strip). Labels include
a "dB" suffix: "0 dB", "-5 dB", "-8 dB", "-11 dB", "-14 dB", "-20 dB", "-23 dB", "-26 dB".

### Visual target:
- No separate darker left strip — the waveform background gradient fills the full width
- Scale labels appear on the right edge of the waveform area, right-aligned with "dB" suffix
- Labels rendered with semi-transparent text overlaid on the waveform background (alpha ~0.7)
- The `kScaleWidth` area is returned to the waveform, increasing waveform display width

### Per-region RMSE measurement (VisualParityAuditor 2026-03-26):
- Left scale strip region: **29.6% RMSE** (highest of all regions)
- Removing the dark left strip is estimated to reduce this region's RMSE by ~12–15 pp

### Required Changes in `WaveformDisplay.cpp`:
1. In `paint()`: remove `bounds.removeFromLeft(kScaleWidth)` — give full width to displayArea
2. In `drawBackground()`: waveform gradient fills the full width (no change needed)
3. In `drawScale()` (or a new `drawScaleOverlay()`):
   - Do NOT fill the scale area with a background colour
   - Draw scale labels on the **right 35 px** of `displayArea`, right-aligned
   - Format: `juce::String(juce::roundToInt(db)) + " dB"` (e.g. "-3 dB", "-6 dB")
   - Colour: `MLIMColours::textSecondary.withAlpha(0.75f)` so waveform content is visible through
4. In `drawCeilingLine()`: remove the scaleArea reference (ceiling label can move to right edge too,
   or be removed — the red ceiling line alone is sufficient)
5. Update `modeSelectorBounds()`: remove `kScaleWidth` offset (now at `x = 4` from left)
6. Remove or repurpose `kScaleWidth` constant — it no longer represents a left strip

## Produces
None

## Consumes
None

## Relevant Files
Modify: `src/ui/WaveformDisplay.cpp` — remove left strip reservation; draw scale labels as right-edge overlay
Modify: `src/ui/WaveformDisplay.h` — remove or repurpose `kScaleWidth` if it's only used for left-strip logic
Read: `src/ui/Colours.h` — `displayGradientTop`, `displayGradientBottom`, `textSecondary`
Read: `/reference-docs/reference-screenshots/prol2-main-ui.jpg` — reference scale label placement

## Acceptance Criteria
- [ ] Run: `cmake --build build --target MLIM_Standalone -j$(nproc) 2>&1 | tail -3` → Expected: `[100%] Built target MLIM_Standalone`
- [ ] Run: visual inspection — no dark left strip visible; waveform background fills full width; dB labels appear on right edge with "dB" suffix
- [ ] Run: RMSE comparison → Expected: left-strip region RMSE < 24% (down from 29.6%)

## Tests
None

## Technical Details
In `paint()`, change:
```cpp
// BEFORE:
auto scaleArea   = bounds.removeFromLeft (kScaleWidth);
auto displayArea = bounds;
// ... pass scaleArea to drawCeilingLine and drawScale

// AFTER:
auto displayArea = bounds;  // full width — no left strip
// ... remove scaleArea from all call sites
```

In `drawScale()` (renamed `drawScaleOverlay()`), replace solid fill with overlay:
```cpp
void WaveformDisplay::drawScaleOverlay(juce::Graphics& g,
                                        const juce::Rectangle<float>& area) const
{
    // No background fill — draw labels overlaid on the waveform
    const float labelW  = 38.0f;
    const float labelX  = area.getRight() - labelW - 2.0f;

    g.setFont(juce::Font(MLIMColours::kFontSizeSmall));
    g.setColour(MLIMColours::textSecondary.withAlpha(0.75f));

    for (int gi = 0; gi < kWaveformGridDBCount; ++gi)
    {
        const float db = MLIMColours::kMeterGridDB[gi];
        float frac = (-db) / kMaxGRdB;
        float y = area.getY() + frac * area.getHeight();
        if (y < area.getY() || y > area.getBottom()) continue;

        juce::String label = (db == 0.0f) ? "0 dB"
                           : juce::String(juce::roundToInt(db)) + " dB";
        g.drawText(label,
                   juce::Rectangle<float>(labelX, y - 6.0f, labelW, 12.0f),
                   juce::Justification::centredRight, false);
    }
}
```

The ceiling label in `drawCeilingLine()` can be moved to the right edge similarly, or removed
(the red ceiling line already conveys the information visually).

## Dependencies
None
