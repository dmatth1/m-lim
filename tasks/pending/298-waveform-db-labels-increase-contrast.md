# Task 298: Waveform dB Scale Labels — Increase Contrast and Font Size

## Description
The dB scale labels drawn on the left edge of the waveform display are currently rendered with:
- Font size: `kFontSizeSmall = 9.0f`
- Color: `textSecondary.withAlpha(0.60f)` = `#9E9E9E` at 60% opacity ≈ `#5F5F5F`

In the Pro-L 2 reference, the dB scale labels (visible as "0 dB", "-3 dB", "-6 dB" etc. on the
left side of the waveform) appear more legible — slightly brighter/larger. The current M-LIM labels
are very faint, especially on the darker waveform background.

**Fix**:
1. Increase font size from 9.0f to 10.0f (1px increase for legibility)
2. Increase alpha from 0.60f to 0.75f (makes labels more readable on the dark background)
3. Use `textPrimary` (#E0E0E0) instead of `textSecondary` (#9E9E9E) as the base color

This keeps labels subtle (75% opacity white text is still unobtrusive) but readable.

## Produces
None

## Consumes
None

## Relevant Files
Modify: `M-LIM/src/ui/WaveformDisplay.cpp` — `drawBackground()` method, lines ~305-324
        (the dB overlay label block)

## Acceptance Criteria
- [ ] Run: `cmake --build /workspace/M-LIM/build --config Release -j$(nproc)` → Expected: exits 0
- [ ] Visual: Screenshot shows dB labels on left edge of waveform that are clearly legible (not washed out)
- [ ] Visual: Labels use approximately 10pt font and appear at 70-80% brightness on the dark background
- [ ] Visual: Grid lines and labels are not distracting — they remain a secondary visual element

## Tests
None

## Technical Details
In `WaveformDisplay::drawBackground()`, change:
```cpp
// Before:
g.setFont (juce::Font (MLIMColours::kFontSizeSmall));
// ...
float alpha = 0.60f;
g.setColour (MLIMColours::textSecondary.withAlpha (alpha));
g.drawText (label, labelRect, juce::Justification::centredLeft, false);

// After:
g.setFont (juce::Font (10.0f));
// ...
float alpha = 0.75f;
g.setColour (MLIMColours::textPrimary.withAlpha (alpha));
g.drawText (label, labelRect, juce::Justification::centredLeft, false);
```

Also consider making the label width slightly wider (40px → 44px) to avoid clipping with the 10pt
font, especially for "-27 dB" labels.

## Dependencies
None
