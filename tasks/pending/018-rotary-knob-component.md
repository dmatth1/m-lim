# Task 018: Rotary Knob Component

## Description
Create the custom rotary knob UI component matching Pro-L 2's visual style — dark circular face, light blue value arc, white pointer tick, with label and value display.

## Produces
Implements: `RotaryKnobInterface`

## Consumes
ColoursDefinition

## Relevant Files
Create: `M-LIM/src/ui/RotaryKnob.h` — class declaration
Create: `M-LIM/src/ui/RotaryKnob.cpp` — implementation
Read: `M-LIM/src/ui/Colours.h` — color constants
Read: `M-LIM/src/ui/LookAndFeel.h` — theme
Read: `SPEC.md` — RotaryKnobInterface

## Acceptance Criteria
- [ ] Run: `cd M-LIM && cmake --build build --target MLIM_VST3 -j$(nproc) 2>&1 | tail -3` → Expected: builds without errors

## Tests
None (visual component — verified by UI parity auditor)

## Technical Details
- Extends juce::Component, contains internal juce::Slider (RotaryHorizontalVerticalDrag style)
- Paint override draws:
  1. Dark circle background (knobFace color)
  2. Light blue arc from start angle to current value angle (knobArc color)
  3. White pointer tick at current angle (knobPointer color)
  4. Label text below knob (textSecondary color)
  5. Value + suffix text below label (textPrimary color)
- Arc range: ~270 degrees (from ~7:30 position to ~4:30 position, matching Pro-L 2 knob style)
- Supports APVTS attachment via juce::AudioProcessorValueTreeState::SliderAttachment
- setRange, setValue, setLabel, setSuffix methods
- onValueChange callback for non-APVTS use
- Mouse drag sensitivity appropriate for knob size

## Dependencies
Requires task 003
