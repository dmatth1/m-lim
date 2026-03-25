# Task 047: Output Ceiling Vertical Slider in Control Strip

## Description
Pro-L 2 uses a vertical slider for Output Ceiling at the right edge of the control strip. Task 025 mentions this control but doesn't specify it should be a vertical slider matching Pro-L 2's visual style. Implement the Output Ceiling as a custom vertical slider with dB readout.

**NOTE:** The Input Gain slider does NOT belong in the control strip. In Pro-L 2, input gain is a tall vertical slider overlaid on the LEFT edge of the waveform display area. See task 058 for correct input gain placement.

## Produces
None

## Consumes
ColoursDefinition
ParameterLayout

## Relevant Files
Modify: `M-LIM/src/ui/ControlStrip.h` — add output ceiling vertical slider member
Modify: `M-LIM/src/ui/ControlStrip.cpp` — implement vertical slider layout and APVTS attachment
Read: `M-LIM/src/ui/Colours.h` — color constants
Read: `M-LIM/src/ui/LookAndFeel.h` — may need drawLinearSlider override

## Acceptance Criteria
- [ ] Run: `cd M-LIM && cmake --build build --target MLIM_VST3 -j$(nproc) 2>&1 | tail -3` → Expected: builds successfully
- [ ] Run: `grep -c "LinearVertical\|LinearBarVertical\|outputCeiling" M-LIM/src/ui/ControlStrip.cpp` → Expected: at least 2

## Tests
None (visual component)

## Technical Details
- Output Ceiling slider: far right of control strip, vertical, range -30 to 0 dB, default -0.1
- Visual style: thin track (dark), fill bar (accent blue), thumb marker
- Numeric dB readout above or below the slider
- Label text: "OUTPUT" (or "CEILING")
- Height should span both rows of the control strip (~120px)
- Attached to APVTS parameter "outputCeiling"
- LookAndFeel::drawLinearSlider override may be needed for styling
- Input Gain is handled by task 058 (waveform overlay), NOT in this control strip

## Dependencies
Requires task 025
