# Task 258: Algorithm Selector — Inactive Button Background Clashes with Control Strip

## Description
The algorithm selector (STYLE) inactive buttons use `buttonBackground = 0xff242424` (very dark
near-black). The control strip behind them has a blue-gray gradient from `controlStripTop = 0xff75809A`
(top) to `controlStripBottom = 0xff3E4255` (bottom). This creates a harsh visual clash where the
dark buttons look like foreign objects on the lighter strip.

In the Pro-L 2 reference (`prol2-main-ui.jpg`, `prol2-features.jpg`), the inactive algorithm buttons
blend with the control strip — they use a darker shade of the strip's blue-gray color, not an
unrelated dark gray. The selected button stands out clearly.

**Fix**: In `AlgorithmSelector::updateButtonStates()`, change the inactive button color from
`MLIMColours::buttonBackground` to a new constant `MLIMColours::algoButtonInactive` (approximately
`0xff303848` — dark desaturated blue-gray that blends with the strip gradient).

Add the new constant to `Colours.h`:
```cpp
const juce::Colour algoButtonInactive { 0xff303848 };  // inactive algo button — dark blue-gray blending with control strip
```

Also adjust the border in `AlgorithmSelector::paint()` — the `panelBorder` (`0xff333333`) around the
whole selector should use a slightly lighter value for better visibility against the strip:
adjust to `MLIMColours::algoButtonInactive.brighter(0.3f)` or use a similar hardcoded value.

## Produces
None

## Consumes
None

## Relevant Files
Modify: `src/ui/Colours.h` — add `algoButtonInactive` constant
Modify: `src/ui/AlgorithmSelector.cpp` — `updateButtonStates()`: use `algoButtonInactive` for
  inactive buttons; also update `paint()` border color
Read:   `src/ui/ControlStrip.cpp` — control strip gradient colors for reference
Read:   `/reference-docs/reference-screenshots/prol2-main-ui.jpg` — reference for expected appearance
Read:   `/reference-docs/reference-screenshots/prol2-features.jpg` — closeup of control strip area

## Acceptance Criteria
- [ ] Run: `cd /workspace/M-LIM && cmake --build build -j$(nproc) 2>&1 | tail -5` →
      Expected: build succeeds
- [ ] Run: `cd /workspace/M-LIM/build && ctest --output-on-failure` →
      Expected: all tests pass
- [ ] Visual check: screenshot of plugin — inactive algorithm buttons should look like a dark
      blue-gray that blends naturally with the control strip gradient, not a stark black/dark-gray

## Tests
None (color styling change — no unit tests)

## Technical Details
In `AlgorithmSelector::updateButtonStates()`, replace:
```cpp
algoButtons_[i]->setColour(juce::TextButton::buttonColourId,
                            isSelected ? MLIMColours::accentBlue
                                       : MLIMColours::buttonBackground);
```
with:
```cpp
algoButtons_[i]->setColour(juce::TextButton::buttonColourId,
                            isSelected ? MLIMColours::accentBlue
                                       : MLIMColours::algoButtonInactive);
```

The `accentBlue = 0xff2D7EE8` for the selected button looks correct and should remain unchanged.
The inactive text color (`textSecondary = 0xff9E9E9E`) is acceptable, but consider using
`MLIMColours::textPrimary.withAlpha(0.7f)` for inactive text to match the reference's lighter
inactive label appearance.

## Dependencies
None
