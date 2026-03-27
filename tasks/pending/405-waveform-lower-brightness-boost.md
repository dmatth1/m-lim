# Task 405: Waveform Lower Zone Brightness Boost (y=65%–100%)

## Description

Pixel analysis shows M-LIM's waveform display is too dark in the lower zone (y=300-390):

**Measured gap at x=300:**
- y=300: M-LIM srgb(110,123,154) vs Reference srgb(144,155,181) → +34R, +32G, +27B gap
- y=350: M-LIM srgb(103,120,158) vs Reference srgb(130,137,165) → +27R, +17G, +7B gap
- y=390: M-LIM srgb(104,121,162) vs Reference srgb(119,125,153) → +15R, +4G, -9B gap

The reference is brighter in the lower portion because output waveform and envelope content
overlays the background there. M-LIM's idle simulation has no specific lower-zone boost.

**Fix**: Add a new brightness-boost gradient pass in the lower 35% of the waveform area
(y=65% to y=100%). Use a neutral/warm gray tone that approximates the output waveform composite.
This will add ~15-25 units of brightness in the y=300-390 region.

## Produces
None

## Consumes
None

## Relevant Files
Modify: `src/ui/WaveformDisplay.cpp` — add new gradient pass after the center tent (around line 355)
Read: `src/ui/Colours.h` — outputWaveform and outputEnvelope color constants

## Acceptance Criteria
- [ ] Run: `cmake --build build --target MLIM_Standalone_Standalone -j$(nproc)` → Expected: exit 0
- [ ] Run wave RMSE → Expected: wave RMSE ≤ 16.72%
- [ ] Run full RMSE → Expected: full RMSE ≤ 19.46%
- [ ] Run: `ls screenshots/task-405-after.png screenshots/task-405-rmse-results.txt` → Expected: files exist

## Tests
None

## Technical Details

**Implementation approach** — add after center tent, before grid lines (~line 356):
```cpp
// Lower idle fill — approximates output waveform/envelope content in lower portion
// Covers y=62%–100%, peak alpha at ~80%, fades to 0 at 62%
{
    const float lTop = area.getY() + area.getHeight() * 0.62f;
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
- If y=390 makes M-LIM too blue, try warmer color #9898A8 (equal R/G/B)

**RMSE methodology**: see task 403 for commands.

## Dependencies
None
