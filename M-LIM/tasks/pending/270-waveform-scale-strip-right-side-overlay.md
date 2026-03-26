# Task 270: WaveformDisplay — Restore dB Scale Labels as Left-Edge Overlay

## Description
Task 267 set `kScaleWidth = 0` to remove the double-scale artifact (left strip). This was
correct, but it also removed the dB scale labels entirely from the waveform display. Now
`drawScale()` draws into a zero-width rectangle and produces nothing visible.

The FabFilter Pro-L 2 reference (v1-0003.png, v1-0005.png) shows dB scale labels drawn
**directly overlaid on the LEFT edge** of the waveform display area, with no separate
background strip. Labels like "0 dB", "-9 dB", "-15 dB", "-20 dB", "-26 dB" are rendered
as semi-transparent text on top of the waveform gradient.

### Visual target:
- No separate background fill behind labels — waveform gradient shows through
- Scale labels appear on the LEFT edge of the waveform area, right-aligned with "dB" suffix
- Labels rendered with semi-transparent text (`textSecondary.withAlpha(0.75f)`)
- `kScaleWidth` remains 0 (no reserved strip) — labels overlay the existing waveform area

### Per-region RMSE measurement (VisualParityAuditor 2026-03-26):
- Waveform area: dB labels are completely absent in current build, reference clearly shows them
- Restoring the overlay labels improves fidelity to the reference

### Required Changes in `WaveformDisplay.cpp`:
1. In `paint()`: `kScaleWidth` is already 0, but `drawScale(g, scaleArea)` receives a zero-width
   rect so nothing is drawn. Change the call to `drawScaleOverlay(g, displayArea)`.
2. In `drawScale()` rename to `drawScaleOverlay()` and replace the background fill + right-align
   logic with a no-fill, left-edge overlay (see Technical Details below).
3. In `drawCeilingLine()`: the `scaleArea` parameter is zero-width and harmless; no change needed.
4. Update `modeSelectorBounds()` if needed — `kScaleWidth` is 0 so this is already correct.

## Produces
None

## Consumes
None

## Relevant Files
Modify: `src/ui/WaveformDisplay.cpp` — update `drawScale()` to draw labels as left-edge overlay (no background fill)
Read: `src/ui/WaveformDisplay.h` — check `kScaleWidth` usage; it is already 0
Read: `src/ui/Colours.h` — `textSecondary` colour constant
Read: `/reference-docs/video-frames/v1-0003.png` — reference showing dB labels overlaid on LEFT of waveform

## Acceptance Criteria
- [ ] Run: `cmake --build build --target MLIM_Standalone -j$(nproc) 2>&1 | tail -3` → Expected: `[100%] Built target MLIM_Standalone`
- [ ] Run: visual inspection — dB labels visible as semi-transparent text overlaid on waveform's left edge; no separate darker background strip

## Tests
None

## Technical Details
`kScaleWidth` is already 0. The `drawScale()` call receives a zero-width `scaleArea`, so nothing
is drawn. Fix by passing `displayArea` (the full waveform area) to a renamed `drawScaleOverlay()`
and drawing labels on the LEFT edge of that area:

```cpp
void WaveformDisplay::drawScaleOverlay(juce::Graphics& g,
                                        const juce::Rectangle<float>& area) const
{
    // No background fill — draw labels overlaid on the waveform left edge
    const float labelW = 38.0f;
    const float labelX = area.getX() + 2.0f;

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
                   juce::Justification::centredLeft, false);
    }
}
```

In `paint()`, change `drawScale(g, scaleArea)` → `drawScaleOverlay(g, displayArea)`.
Update the declaration in `WaveformDisplay.h` accordingly.

## Dependencies
None
