# Task 286: RotaryKnob — Add Min/Max Range Labels Flanking Value Display

## Description
In Pro-L 2, each rotary knob shows its parameter range bounds as small labels flanking the
current value display (e.g., Lookahead shows "0 ms" on the left and "5 ms" on the right
underneath the knob). M-LIM's RotaryKnob only shows the current value centred below the knob.

Adding range labels improves visual parity with Pro-L 2 and gives users immediate visual
context for the knob's range without needing to hover.

### Required changes in `RotaryKnob.cpp`:

In `RotaryKnob::paint()`, replace the current value-label drawing block (around line 106–112)
with a three-part layout: min label on the left, current value in the centre, max label on the right.

```cpp
    // Value row: [min label] [current value] [max label]
    const float valueRowY = (float)(int)(knobY + knobSize + 2);
    const float valueRowH = valueH;
    const float colW = knobSize / 3.0f;

    // Min label (left third)
    g.setColour (MLIMColours::textSecondary.withAlpha (0.65f));
    g.setFont (juce::Font (MLIMColours::kFontSizeSmall - 1.0f));
    juce::String minStr = juce::String (slider.getMinimum(), 0) + " " + suffixText;
    g.drawFittedText (minStr,
                      juce::Rectangle<int> ((int)knobX, (int)valueRowY, (int)colW, (int)valueRowH),
                      juce::Justification::centredLeft, 1);

    // Current value (centre third)
    g.setColour (MLIMColours::textPrimary);
    g.setFont (juce::Font (MLIMColours::kFontSizeMedium));
    g.drawFittedText (cachedValueStr_,
                      juce::Rectangle<int> ((int)(knobX + colW), (int)valueRowY,
                                            (int)colW, (int)valueRowH),
                      juce::Justification::centred, 1);

    // Max label (right third)
    g.setColour (MLIMColours::textSecondary.withAlpha (0.65f));
    g.setFont (juce::Font (MLIMColours::kFontSizeSmall - 1.0f));
    juce::String maxStr = juce::String (slider.getMaximum(), 0) + " " + suffixText;
    g.drawFittedText (maxStr,
                      juce::Rectangle<int> ((int)(knobX + 2.0f * colW), (int)valueRowY,
                                            (int)colW, (int)valueRowH),
                      juce::Justification::centredRight, 1);
```

### Visual target:
- Below each knob: `0 ms` [current value centred] `5 ms` (for Lookahead)
- Range labels are dimmer (textSecondary at 65% alpha) so the current value stands out
- Range label font is 1pt smaller than current value font

## Produces
None

## Consumes
None

## Relevant Files
Modify: `M-LIM/src/ui/RotaryKnob.cpp` — replace single centred value label with three-column min/value/max layout
Read: `M-LIM/src/ui/RotaryKnob.h` — class structure, `slider`, `suffixText`, `cachedValueStr_` fields
Read: `M-LIM/src/ui/Colours.h` — font size constants (`kFontSizeSmall`, `kFontSizeMedium`), `textSecondary`, `textPrimary`

## Acceptance Criteria
- [ ] Run: `cmake --build build --target MLIM_Standalone -j$(nproc) 2>&1 | tail -3` → Expected: build succeeds, exit 0
- [ ] Run: `cd build && ctest --output-on-failure 2>&1 | tail -3` → Expected: all tests pass
- [ ] Visual check: each rotary knob in the control strip shows min value on the bottom-left, current value centred, and max value on the bottom-right below the knob face

## Tests
None

## Technical Details
- `slider.getMinimum()` and `slider.getMaximum()` return the range bounds as double
- Use precision 0 for range labels (integer display: "0 ms" not "0.0 ms")
- The `suffixText` (e.g., "ms", "%") is appended to both range labels
- `kFontSizeSmall = 9.0f`; range label font = 8.0f (kFontSizeSmall - 1.0f)
- The centre column still uses `cachedValueStr_` (which already includes suffix with 1 decimal: "1.0 ms")
- Knob width `knobSize` is computed from available bounds; `colW = knobSize / 3.0f` fits within the same footprint

## Dependencies
None
