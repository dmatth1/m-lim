# Task 033: Input Gain and Output Ceiling as Vertical Sliders

## Description
Pro-L 2 uses prominent tall vertical sliders (not rotary knobs) for Input Gain and Output Ceiling at the edges of the control strip. Task 025 mentions these controls but doesn't specify they should be vertical sliders matching Pro-L 2's visual style. Implement them as custom vertical sliders with dB readout.

## Produces
None

## Consumes
ColoursDefinition
ParameterLayout

## Relevant Files
Modify: `M-LIM/src/ui/ControlStrip.h` — add vertical slider members instead of knobs for gain/ceiling
Modify: `M-LIM/src/ui/ControlStrip.cpp` — implement vertical slider layout and APVTS attachment
Read: `M-LIM/src/ui/Colours.h` — color constants
Read: `M-LIM/src/ui/LookAndFeel.h` — may need drawLinearSlider override

## Acceptance Criteria
- [ ] Run: `cd M-LIM && cmake --build build --target MLIM_VST3 -j$(nproc) 2>&1 | tail -3` → Expected: builds successfully
- [ ] Run: `grep -c "LinearVertical\|LinearBarVertical" M-LIM/src/ui/ControlStrip.cpp` → Expected: at least 2

## Tests
None (visual component)

## Technical Details
- Input Gain slider: far left of control strip, vertical, range -12 to +36 dB
- Output Ceiling slider: far right of control strip, vertical, range -30 to 0 dB
- Visual style: thin track (dark), fill bar (accent blue), thumb marker
- Numeric dB readout above or below the slider
- Label text: "INPUT" and "OUTPUT" (or "GAIN" and "CEILING")
- Height should span both rows of the control strip (~120px)
- Attached to APVTS parameters "inputGain" and "outputCeiling"
- LookAndFeel::drawLinearSlider override may be needed for styling

## Dependencies
Requires task 025
