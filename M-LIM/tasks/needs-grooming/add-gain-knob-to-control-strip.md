# Task: Add GAIN Knob to Control Strip (Matching Reference Layout)

## Description
The Pro-L 2 reference shows a GAIN knob as the first control in the control strip (far left), directly before the STYLE selector. In M-LIM, the input gain control is implemented as a floating badge in the waveform display area (bottom-left corner), which is both visually different from the reference and reduces the waveform display area.

To improve visual parity:
1. Move the input gain control from the waveform overlay to the control strip, positioned at the far left before the STYLE selector
2. Render it as a standard RotaryKnob (matching the LOOKAHEAD, ATTACK, RELEASE knobs)
3. Remove the floating gain badge from the waveform display area

This affects the control strip RMSE (19%) and waveform RMSE (16.5%) by changing the layout to match the reference more closely.

## Produces
None

## Consumes
None

## Relevant Files
Modify: `src/PluginEditor.cpp` — move gain slider from waveform overlay to control strip
Modify: `src/PluginEditor.h` — update component declarations
Modify: `src/ui/ControlStrip.cpp` — add GAIN knob at far left position
Modify: `src/ui/ControlStrip.h` — add gain knob member
Read: `src/ui/RotaryKnob.h` — knob component to reuse

## Acceptance Criteria
- [ ] Run: `cmake --build build --target MLIM_Standalone -j$(nproc)` → Expected: builds successfully
- [ ] Run: visual comparison → Expected: GAIN knob appears at far left of control strip, no floating badge in waveform

## Tests
None

## Technical Details
- The GAIN knob uses the existing `inputGain` parameter from APVTS
- Label "GAIN" above the knob, matching other knob labels
- Remove `inputGainSlider_`, `gainLabel_`, and `inputGainValueLabel_` from PluginEditor
- The knob should show range -12 to +36 dB with default 0.0

## Dependencies
None
