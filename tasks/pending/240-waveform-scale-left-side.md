# Task 240: Waveform dB Scale — Move to Left-Side Overlay

## Description
In the Pro-L 2 reference, the dB scale labels (0 dB, -3 dB, -6 dB … -27 dB) are drawn
**inside the waveform display area on its LEFT edge**, overlaid on the waveform content.

Currently, `WaveformDisplay::paint()` in `WaveformDisplay.cpp` reserves a strip from
the **RIGHT** edge of its bounds for the scale, then draws `drawScale()` there. This
creates a visible right-side strip with dark background that narrows the waveform area
and differs from the reference layout.

**Fix**: Move the scale strip from the right side to the left side:
1. In `WaveformDisplay::paint()`, change `bounds.removeFromRight(kScaleWidth)` to
   `bounds.removeFromLeft(kScaleWidth)`.
2. Pass the left-side `scaleArea` to `drawScale()` (and `drawCeilingLine()` if it uses
   scaleArea for ceiling label positioning).
3. In `drawScale()`, draw labels right-aligned in the strip so they appear adjacent to
   the waveform content.
4. The ceiling label (in `drawCeilingLine`) currently uses `scaleArea` — update it to
   use the left-side scale area, and position the label on the left of the waveform.

The scale strip width (`kScaleWidth`) can stay the same. The input gain overlay in
`PluginEditor.cpp` (`gainOverlay`) uses `bounds.withWidth(kGainSliderW)` which
references the waveform bounds **after** the TopBar and inputMeter are removed — that
overlay should remain unaffected since it references the left edge of the waveform
display differently.

## Produces
None

## Consumes
None

## Relevant Files
Modify: `src/ui/WaveformDisplay.cpp` — change scale strip to left side; update drawScale, drawCeilingLine
Read:   `src/ui/WaveformDisplay.h` — kScaleWidth constant and method signatures
Read:   `src/PluginEditor.cpp` — gain slider overlay bounds (verify not broken by this change)

## Acceptance Criteria
- [ ] Run: build, launch standalone, observe waveform display → Expected: dB scale labels (-0, -3, -6 … -27) appear on LEFT edge of waveform area, not the right
- [ ] Run: build, launch standalone → Expected: waveform area shows no empty right-side strip; the full waveform width (minus scale) is used for content
- [ ] Run: `cd build && ctest --output-on-failure` → Expected: all tests pass

## Tests
None (visual layout change; no testable logic)

## Technical Details
In `WaveformDisplay.cpp`, `paint()`:
```cpp
// BEFORE:
auto scaleArea = bounds.removeFromRight(kScaleWidth);
// AFTER:
auto scaleArea = bounds.removeFromLeft(kScaleWidth);
```
In `drawScale()`, labels should be right-aligned within the left strip so they read
clearly next to the waveform. Use `juce::Justification::centredRight` for the label
text and adjust the label x-offset accordingly.

In `drawCeilingLine()`, the ceiling label at `scaleArea.getX() + 2` needs updating to
position correctly at the left edge.

## Dependencies
None
