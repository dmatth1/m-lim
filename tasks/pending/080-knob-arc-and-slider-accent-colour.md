# Task 080: Knob Arc and Slider Accent Colour Doesn't Match Pro-L 2

## Description
`MLIMColours::knobArc` (`0xff4FC3F7` — bright cyan/ice blue) is used for:
1. The rotary knob **value arc** (the filled arc from start position to current value)
2. **Linear slider fill** colour (via `Slider::trackColourId` and `drawLinearSlider`)
3. The `AlgorithmSelector` combo box **text colour**

None of these match the Pro-L 2 reference:

- **Rotary knob arc**: In Pro-L 2, knobs show a very subtle or absent arc fill — the position is communicated primarily by the white pointer direction. The arc (if present) appears as a subdued, slightly lighter shade of the blue-grey background, NOT bright cyan.
- **Algorithm selector text**: In the reference, the algorithm name ("Modern", "Allround", etc.) appears in **white/light grey text** on a dark blue-grey background — NOT bright cyan.
- **Linear slider fill**: In Pro-L 2 control sliders (e.g. output ceiling), the filled track portion uses the `accentBlue` (`0xff2196F3`) rather than cyan.

Reference: `/reference-docs/reference-screenshots/prol2-features.jpg` (knobs — no bright arc visible), `/reference-docs/video-frames/v1-0020.png` (algorithm selector shows white text).

## Produces
None

## Consumes
ColoursDefinition
LookAndFeelDefinition

## Relevant Files
Modify: `M-LIM/src/ui/Colours.h` — change `knobArc` to a subtle accent; add `sliderFill` accent constant
Modify: `M-LIM/src/ui/LookAndFeel.cpp` — use `accentBlue` for slider track fill, use subtle colour for knob arc track
Modify: `M-LIM/src/ui/AlgorithmSelector.cpp` — change text colour from `knobArc` to `textPrimary`

## Acceptance Criteria
- [ ] Run: `cd M-LIM && cmake --build build --target MLIM_VST3 -j$(nproc) 2>&1 | tail -3` → Expected: builds successfully
- [ ] Run: `grep "knobArc" M-LIM/src/ui/Colours.h` → Expected: NOT `4FC3F7` (bright cyan gone)
- [ ] Run: `grep "textColourId.*knobArc" M-LIM/src/ui/AlgorithmSelector.cpp` → Expected: no match (text colour changed to textPrimary)

## Tests
None (visual styling)

## Technical Details

**Change 1 — `Colours.h`:**
```cpp
// Old (wrong):
const juce::Colour knobArc { 0xff4FC3F7 };  // bright cyan

// New:
const juce::Colour knobArc  { 0xff5C6A84 };  // subtle blue-grey arc — matches knob surround
const juce::Colour sliderFill { 0xff2196F3 }; // accent blue for linear slider fills
```

**Change 2 — `AlgorithmSelector.cpp`:**
```cpp
// Old (wrong):
comboBox.setColour(juce::ComboBox::textColourId, MLIMColours::knobArc); // was cyan

// New:
comboBox.setColour(juce::ComboBox::textColourId, MLIMColours::textPrimary); // white/light
```

**Change 3 — `LookAndFeel.cpp`:**
- In the Slider colour setup: change `Slider::trackColourId` from `knobArc` to `sliderFill`
- In `drawLinearSlider`: use `MLIMColours::sliderFill` (accent blue) for the filled portion
- In `drawRotarySlider`: for the value arc, keep using `knobArc` but it is now the subtle blue-grey

**Note:** Task 044 (active — fixes arc angular range) also modifies `LookAndFeel.cpp`. Coordinate with or merge after task 044 completes to avoid conflict.

## Dependencies
Requires task 003 (Colours.h), Requires task 019 (AlgorithmSelector)
Coordinate with task 044 (arc range fix — currently active)
