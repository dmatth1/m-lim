# Task: Level Meter Idle Gradient — Simulate Active Fill at ~-6 dBFS

## Description
The reference Pro-L 2 image shows active audio with level meters filled to approximately -6 dBFS. Our idle state (no audio) shows a very subtle gradient that results in very dark meters (e.g., RGB 62, 70, 90 at mid-height). The reference shows warm amber/yellow at the warning zone (top ~10% of filled area) and neutral steel-blue in the safe zone (bottom ~75% of meter).

The current idle gradient in `LevelMeter.cpp::drawChannel()` has very low alpha at warning/danger zone positions (0.03%) — essentially invisible. This creates a large RMSE gap because the reference shows vivid warm colors in those areas.

**Proposed change**: Redesign the idle structural gradient to simulate the visual appearance of a meter at ~-6 dBFS:
- Top ~10% (above simulated fill level): dark track background
- ~10% to ~15%: warm red/danger zone (alpha ~0.65)
- ~15% to ~25%: warm amber/warning zone (alpha ~0.65)
- ~25% to ~90%: steel-blue safe zone (alpha ~0.88)
- ~90% to 100%: bright blue (alpha ~0.88)

**Important**: Keep warning/danger zone alphas moderate (≤0.70) so that when real audio is active, the unfilled region still appears appropriately dark (the slight warm tint from idle gradient is subtle enough not to interfere with active meter readability).

**Note**: This task should be done AFTER the meterSafe lightening task (which changes the color used at position 0.85 and 1.0 of the idle gradient).

## Produces
None

## Consumes
None

## Relevant Files
Modify: `src/ui/LevelMeter.cpp` — redesign idle structural gradient in `drawChannel()` method (lines ~95-145)
Read: `src/ui/Colours.h` — color constants used in gradient

## Acceptance Criteria
- [ ] Run: `cmake --build build --target MLIM_Standalone -j$(nproc)` → Expected: build succeeds, exit 0
- [ ] Run: launch app, capture screenshot, crop left meter zone → Expected: meter shows warm colors in the upper 20% and blue-gray in the lower 80%, similar to active Pro-L 2 appearance
- [ ] Run: RMSE measurement → Expected: Left zone RMSE < 24% (was 26.22%)

## Tests
None

## Technical Details
Replace the idle gradient block in `LevelMeter.cpp::drawChannel()` with:

```cpp
{
    // Idle structural gradient: simulate active audio at ~-6 dBFS
    // Reference Pro-L 2 shows filled meter with warm zone (top) and safe zone (bottom)
    const float simFillTop = barTop + barH * 0.10f;  // 10% = unfilled (dark)
    const float dangerY    = barTop + barH * 0.15f;  // danger zone at 10-15%
    const float warnY      = barTop + barH * 0.25f;  // warning zone at 15-25%

    juce::ColourGradient idleGrad (
        MLIMColours::barTrackBackground,                     0.0f, barTop,
        MLIMColours::barTrackBackground,                     0.0f, simFillTop,
        false);
    idleGrad.addColour ((simFillTop - barTop) / barH + 0.001f,
                        MLIMColours::meterDanger.withAlpha (0.65f));
    idleGrad.addColour ((dangerY - barTop) / barH,
                        MLIMColours::meterWarning.withAlpha (0.65f));
    idleGrad.addColour ((warnY - barTop) / barH,
                        MLIMColours::grMeterLow.withAlpha (0.60f));
    idleGrad.addColour (0.85f,
                        MLIMColours::meterSafe.withAlpha (0.88f));
    idleGrad.addColour (1.0f,
                        MLIMColours::meterSafe.brighter (0.3f).withAlpha (0.88f));

    g.setGradientFill (idleGrad);
    g.fillRect (bar);
}
```

Remove the segment separators that draw over the idle gradient (or reduce their opacity) if they obscure the warm colors.

Expected per-pixel improvement at danger zone (10-15% from top):
- Current: RGB(42, 40, 56) (essentially background)
- Target: RGB(~200, 60, 60) (red at ~65% alpha over background)
- Reference: RGB(~216, 60, 60) (active danger fill)

Expected RMSE improvement: ~1.5-3.0pp for Left zone, ~0.8-1.5pp for Full.

## Dependencies
Requires meterSafe lightening task (do that first to set the correct color for the safe zone portion)
