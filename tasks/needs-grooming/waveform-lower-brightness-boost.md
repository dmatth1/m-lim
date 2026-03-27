# Task: Waveform Lower Zone Brightness Boost (y=65%–100%)

## Description

Pixel analysis shows M-LIM's waveform display is too dark in the lower zone (y=300-390):

**Measured gap at x=300:**
- y=300: M-LIM srgb(110,123,154) vs Reference srgb(144,155,181) → +34R, +32G, +27B gap
- y=350: M-LIM srgb(103,120,158) vs Reference srgb(130,137,165) → +27R, +17G, +7B gap
- y=390: M-LIM srgb(104,121,162) vs Reference srgb(119,125,153) → +15R, +4G, -9B gap

The reference is brighter in the lower portion because the OUTPUT WAVEFORM and OUTPUT ENVELOPE
(amber/cream line at full loudness level) overlay the background there. M-LIM's idle simulation
doesn't have a specific lower-zone boost — the existing idle fill (fillFrac=0.56) provides blue-gray
color but at a relatively low opacity at y=65-80%.

**Reference background** (from Pro-L 2 video frame, no waveform content areas):
- y=69% (y=250 in frame): srgb(95,103,127)
- y=81% (y=290 in frame): srgb(84,89,115)

These background values are DARKER than the reference screenshot at the same positions — confirming
the lower zone in the reference screenshot is brightened by the output waveform/envelope content,
not the background gradient itself.

**Fix**: Add a new brightness-boost gradient pass in the lower 35% of the waveform area
(y=65% to y=100%). Use a neutral/warm gray tone that approximates the output waveform composite.
This will add ~15-25 units of brightness in the y=300-390 region.

## Relevant Files

Modify: `src/ui/WaveformDisplay.cpp` — add new gradient pass after the center tent (around line 355)
Read: `src/ui/Colours.h` — outputWaveform and outputEnvelope color constants

## Acceptance Criteria

- [ ] Run RMSE on wave region → Expected: wave RMSE ≤ 16.72%
- [ ] Run RMSE on full image → Expected: full RMSE ≤ 19.46%
- [ ] Save screenshot and RMSE to `screenshots/task-NNN-after.png` and `screenshots/task-NNN-rmse-results.txt`

## Tests
None

## Technical Details

**Implementation approach** — add after center tent, before grid lines (~line 356):
```cpp
// Lower idle fill — approximates output waveform/envelope content in lower portion
// Covers y=62%–100%, peak alpha at ~80%, fades to 0 at 62%
// Addresses 15-34 unit brightness gap in lower zone vs reference
{
    const float lTop = area.getY() + area.getHeight() * 0.62f;
    // Use neutral gray-blue to approximate output waveform composite
    juce::Colour lFill { 0xff9298B0 };  // neutral medium blue-gray

    juce::ColourGradient lGrad (
        lFill.withAlpha (0.0f),   0.0f, lTop,
        lFill.withAlpha (0.35f),  0.0f, area.getBottom(),
        false);
    g.setGradientFill (lGrad);
    g.fillRect (area.getX(), lTop, area.getWidth(), area.getBottom() - lTop);
}
```

**Tuning guidance**:
- Start with peak alpha 0.35 and measure wave RMSE
- Try 0.25, 0.40, 0.45 and pick best
- The fill color #9298B0 provides more R/G which helps match the warmer reference at the bottom
- The y=390 reference shows LESS BLUE than M-LIM (-9B); a more neutral color will help
- If at y=390 the lower fill makes M-LIM too blue, try warmer color like #9898A8 (equal R/G/B)

**RMSE methodology** (from task-397):
```bash
convert /tmp/mlim.png -crop 640x500+0+0 +repage /tmp/mlim-wave.png
convert /tmp/ref.png  -crop 640x500+0+0 +repage /tmp/ref-wave.png
compare -metric RMSE /tmp/mlim-wave.png /tmp/ref-wave.png /dev/null 2>&1
```

Also measure the "left" region since the lower fill extends to the right edge of the waveform
at x=640 (which is in the 80x500+640+0 "left" region):
```bash
convert /tmp/mlim.png -crop 80x500+640+0 +repage /tmp/mlim-left.png
convert /tmp/ref.png  -crop 80x500+640+0 +repage /tmp/ref-left.png
compare -metric RMSE /tmp/mlim-left.png /tmp/ref-left.png /dev/null 2>&1
```

## Dependencies
None
