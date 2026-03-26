# Task 285: ControlStrip — Remove CHANNEL LINKING Rounded-Box Overlay

## Description
`ControlStrip::paint()` currently draws a rounded rectangle overlay (fill + border) around the
CHANNEL LINKING knob pair. The Pro-L 2 reference (`prol2-features.jpg`) shows no such box — the
CHANNEL LINKING section is delineated only by the "CHANNEL LINKING" label above and the visual
arrangement of the knobs themselves.

Removing the box makes the control strip visually consistent with the reference and removes an
extra visual element that clutters the knob row.

## Produces
None

## Consumes
None

## Relevant Files
Modify: `M-LIM/src/ui/ControlStrip.cpp` — `paint()` block labelled "Draw CHANNEL LINKING section
overlay" (approx lines 427–444)
Read: `/reference-docs/reference-screenshots/prol2-features.jpg` — shows knobs without a
surrounding box

## Acceptance Criteria
- [ ] Build: `cmake --build /workspace/M-LIM/build --config Release -j$(nproc) --target MLIM_Standalone` → Expected: exits 0
- [ ] Visual: Screenshot of control strip shows CHANNEL LINKING knobs with no rounded-rectangle
  border box; the "CHANNEL LINKING" label above the knobs is still present.

## Tests
None

## Technical Details
Remove this block from `ControlStrip::paint()`:
```cpp
// Draw CHANNEL LINKING section overlay (always visible — Pro-L 2 parity)
{
    auto linkBounds = channelLinkTransientsKnob_.getBounds()
                          .getUnion(channelLinkReleaseKnob_.getBounds())
                          .expanded(4, 2);
    g.setColour(MLIMColours::panelOverlay);
    g.fillRoundedRectangle(linkBounds.toFloat(), 4.0f);
    g.setColour(MLIMColours::panelBorder);
    g.drawRoundedRectangle(linkBounds.toFloat(), 4.0f, 1.0f);
    // ... "CHANNEL LINKING" label draw ...
}
```
Keep only the "CHANNEL LINKING" text label drawing, removing the fill/border rectangle.

## Dependencies
None
