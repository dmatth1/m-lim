# Task 306: Waveform dB Overlay Labels — Reduce Opacity to Match Reference

## Description
The waveform `drawBackground()` method draws dB scale labels ("-3 dB", "-6 dB", etc.) overlaid
on the left edge of the waveform display area at `alpha = 0.75` (75%) of `textPrimary` colour.

At idle (no audio), these labels are clearly visible as bright white text against the medium
blue-gray background. In the reference Pro-L 2 screenshot (which has dense signal bars covering
most of the waveform), the label area measures:
- Reference at label position (x=2, y=50): `#3A232A` (very dark, nearly black)
- M-LIM at label position: `#6B7783` (medium blue-gray with label text contribution)

The labels at 75% opacity are too bright and contribute to RMSE in the waveform area. Reducing
opacity to 0.35 (35%) makes them more subtle — still legible but not a dominant visual element.

This matches the Pro-L 2 style where labels are present but unobtrusive.

## Produces
None

## Consumes
None

## Relevant Files
Modify: `M-LIM/src/ui/WaveformDisplay.cpp` — `drawBackground()`: change `float alpha = 0.75f`
  to `float alpha = 0.35f` (line ~322)

## Acceptance Criteria
- [ ] Run: `cmake --build /workspace/M-LIM/build --config Release -j$(nproc) --target MLIM_Standalone 2>&1 | tail -3` → Expected: exit 0
- [ ] Visual: Launch standalone, screenshot. dB labels ("-3 dB", "-6 dB", etc.) should still be
  visible on close inspection but clearly more subtle than before — they should not stand out
  prominently against the waveform background.

## Tests
None

## Technical Details
In `WaveformDisplay.cpp` `drawBackground()`, find the block that draws dB overlay labels:
```cpp
float alpha = 0.75f;
g.setColour (MLIMColours::textPrimary.withAlpha (alpha));
g.drawText (label, labelRect, juce::Justification::centredLeft, false);
```

Change `0.75f` to `0.35f`.

No other changes needed — the label positions and content remain the same.

## Dependencies
None
