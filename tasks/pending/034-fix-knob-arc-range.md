# Task 034: Fix Rotary Knob Arc Range to Match Pro-L 2

## Description
Task 018 specifies a 210-degree arc for rotary knobs, but Pro-L 2 knobs use approximately 270-280 degrees of rotation (from roughly 8 o'clock to 4 o'clock). A 210-degree arc is noticeably too narrow and will look visually wrong compared to the reference.

## Produces
None

## Consumes
RotaryKnobInterface

## Relevant Files
Modify: `M-LIM/src/ui/RotaryKnob.cpp` — change arc start/end angles
Modify: `M-LIM/src/ui/LookAndFeel.cpp` — update drawRotarySlider angle range if implemented there

## Acceptance Criteria
- [ ] Run: `cd M-LIM && cmake --build build --target MLIM_VST3 -j$(nproc) 2>&1 | tail -3` → Expected: builds successfully
- [ ] Run: `grep -E "270|280|4\.7|4\.8" M-LIM/src/ui/RotaryKnob.cpp M-LIM/src/ui/LookAndFeel.cpp` → Expected: at least 1 match showing corrected arc range

## Tests
None (visual component)

## Technical Details
- Change arc from 210 degrees to ~270 degrees
- Start angle: approximately -135 degrees from 12 o'clock (= 7:30 position)
- End angle: approximately +135 degrees from 12 o'clock (= 4:30 position)
- In radians: start ≈ -3π/4 from top, end ≈ +3π/4 from top
- JUCE rotary slider uses `rotaryStartAngle` and `rotaryEndAngle` parameters
- Typical JUCE values: startAngle = juce::MathConstants<float>::pi * 1.25f, endAngle = juce::MathConstants<float>::pi * 2.75f (for 270-degree range)
- This is a small numeric change but has significant visual impact

## Dependencies
Requires task 018
