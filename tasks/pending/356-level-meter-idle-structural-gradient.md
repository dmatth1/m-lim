# Task 356: LevelMeter Idle Structural Gradient

## Description

At idle (no audio), M-LIM's LevelMeter bars are rendered as a near-black background
(0xff181818, ~10% brightness) with only hairline segment separators visible.

The Pro-L 2 reference shows the level meter column with a clearly visible structural
gradient even when no signal is present: faint warm-blue tints at lower dB zones fading to
a slight warm/orange tint toward the top, giving the meter column a "alive" appearance.

Fix: in `LevelMeter.cpp::drawChannel()`, after filling the barTrackBackground, overlay a
very low-alpha (~12–18%) version of the full zone gradient over the entire bar. This means:
- Always draw the inactive gradient (red top → yellow mid → blue bottom) at 12–18% alpha
- Then draw the active fill at full alpha on top (as now)
- The result: at idle the meter has visible structure; during active audio the gradient is
  covered by the full-brightness fill.

## Produces
None

## Consumes
None

## Relevant Files
Modify: `src/ui/LevelMeter.cpp` — `drawChannel()` method, after `barTrackBackground` fill
Read: `src/ui/Colours.h` — meterDanger, meterWarning, meterSafe color values

## Acceptance Criteria
- [ ] Run: `cmake --build build -j$(nproc) 2>&1 | grep -c "error:"` → Expected: `0`
- [ ] Run: visual inspection of screenshot — meter bars should show a faint multi-color tint at
  idle (not pure black). Light blue at bottom zone, slight warm tint at top zone.
- [ ] Run: full image RMSE must not regress beyond +0.3% from 22.08% baseline
  → Expected: ≤ 22.38%

## Tests
None

## Technical Details

In `src/ui/LevelMeter.cpp`, inside `drawChannel()`, after the line:
```cpp
g.setColour (MLIMColours::barTrackBackground);
g.fillRect (bar);
```

Add an idle structural gradient:
```cpp
// Idle structural gradient — gives the meter visual presence at silence.
// Same colour stops as the active fill but at very low alpha (~15%).
{
    const float barTop2 = bar.getY();
    const float barH2   = bar.getHeight();

    const float normWarn2   = dbToNorm (kWarnDB);
    const float normDanger2 = dbToNorm (kDangerDB);
    const float dangerBot2  = barTop2 + barH2 * (1.0f - normDanger2);
    const float warnBot2    = barTop2 + barH2 * (1.0f - normWarn2);

    juce::ColourGradient idleGrad (
        MLIMColours::meterDanger.withAlpha (0.15f),             0.0f, barTop2,
        MLIMColours::meterSafe.darker (0.3f).withAlpha (0.15f), 0.0f, barTop2 + barH2,
        false);
    idleGrad.addColour ((dangerBot2 - barTop2) / barH2,
                        MLIMColours::meterWarning.withAlpha (0.15f));
    idleGrad.addColour ((warnBot2   - barTop2) / barH2,
                        MLIMColours::meterSafe.brighter (0.15f).withAlpha (0.15f));

    g.setGradientFill (idleGrad);
    g.fillRect (bar);
}
```

Place this block BEFORE the "filled level portion" section (the `if (fillH > 0.0f)` block)
and AFTER the segment separator lines. The active fill will then paint on top at full opacity,
completely hiding the idle gradient in active regions.

Tune the alpha value (0.15f ≈ 15%) to taste — keep it subtle so it reads as structure, not
as a false level indication. Range 0.10–0.20f is acceptable.

**RMSE measurement (CORRECT methodology):**
```bash
convert /reference-docs/reference-screenshots/prol2-main-ui.jpg \
    -crop 1712x1073+97+32 +repage -resize 900x500! screenshots/task-356-ref.png

compare -metric RMSE screenshots/task-356-mlim.png screenshots/task-356-ref.png \
    screenshots/task-356-diff.png 2>&1
```

## Dependencies
None
