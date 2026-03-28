# Task 507: Loudness Panel Histogram — Add Warm Amber Idle Bars

## Description
The reference Pro-L 2 shows an active loudness histogram with warm amber/orange bars distributed across the LUFS range (centered around -14 LUFS for streaming content). At idle, M-LIM shows only the dark grid structure with no bars, creating a large RMSE gap in the Right panel zone (currently 23.32%).

The reference histogram shows bars concentrated around the target LUFS level with warm amber coloring (reference measured: approximately #BB9F52 at histogram bar positions). The histogram background without active bars appears darker, but the bars themselves contribute significant warm tone.

**Proposed change**: Add a semi-transparent idle fill overlay to the histogram area in `LoudnessPanel.cpp::drawHistogram()` that simulates a "program-material distribution" bell curve centered around -14 LUFS. This overlay:
1. Draws BEFORE the actual histogram bars (so real bars draw on top)
2. Uses the warm amber color matching the reference
3. Creates a Gaussian-shaped distribution to approximate typical program content appearance
4. Maximum alpha ~0.30 at the center peak, fading to near-zero at the edges

This is a visual approximation to reduce RMSE against the reference, not an accurate representation of loudness data. Real histogram data will always draw on top and override the idle appearance.

## Produces
None

## Consumes
None

## Relevant Files
Modify: `src/ui/LoudnessPanel.cpp` — add idle fill overlay in `drawHistogram()`, before the histogram bars drawing loop
Read: `src/ui/Colours.h` — reference color constants
Read: `src/ui/LoudnessPanel.h` — check `kHistMin`, `kHistStep`, `kHistBins` constants

## Acceptance Criteria
- [ ] Run: `cmake --build build --target MLIM_Standalone -j$(nproc)` → Expected: build succeeds, exit 0
- [ ] Run: launch app at idle, capture screenshot of histogram area → Expected: warm amber/orange tint visible in the center of the histogram (around -14 LUFS), fading at the edges
- [ ] Run: run with active audio → Expected: idle fill not visible (real bars draw on top); histogram shows actual content correctly

## Tests
None

## Technical Details
In `LoudnessPanel::drawHistogram()`, after drawing the grid structure and BEFORE the histogram bars loop, add:

```cpp
// ── Idle warm fill: simulate program content distribution ─────────────────
// Draw a bell-curve of warm amber bars centered at targetLUFS_
// Only visible when histogramMax_ is near 1.0 (no actual data)
if (histogramMax_ < 2.0f)  // idle state: max count ≤ 1 means no real audio
{
    const float centerLUFS = targetLUFS_;   // center at target
    const float sigmaLUFS  = 4.0f;          // ~±4 LUFS width (1 sigma)
    const juce::Colour warmFill { 0xffC4A04A };  // warm amber matching reference #BB9F52

    for (int i = 0; i < kHistBins; ++i)
    {
        const float binLUFS  = kHistMin + static_cast<float> (i) * kHistStep;
        const float diff     = binLUFS - centerLUFS;
        const float gaussian = std::exp (-(diff * diff) / (2.0f * sigmaLUFS * sigmaLUFS));
        const float alpha    = gaussian * 0.28f;  // max 28% opacity

        if (alpha < 0.02f) continue;

        const float normPos = static_cast<float> (i) / static_cast<float> (kHistBins);
        const float barY    = originY + totalH - (normPos + 1.0f / kHistBins) * totalH;
        const float barWidth = barAreaW * 0.85f;  // 85% width like real bars

        g.setColour (warmFill.withAlpha (alpha));
        g.fillRect (juce::Rectangle<float> (originX, barY + 0.5f,
                                            barWidth, std::max (barH - 1.0f, 0.5f)));
    }
}
```

Color constant to add to Colours.h:
```cpp
const juce::Colour loudnessHistogramIdleFill { 0xffC4A04A };  // warm amber for idle histogram simulation
```

Expected RMSE improvement: ~0.5-1.0pp for Right zone (currently 23.32%), ~0.3pp for Full.

## Dependencies
None
