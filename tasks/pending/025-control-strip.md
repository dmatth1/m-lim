# Task 025: Control Strip Component

## Description
Create the bottom control strip containing all main controls: algorithm selector, lookahead/attack/release knobs, channel linking knobs, and the bottom status bar with toggles (TP, oversampling, dither, DC, bypass, unity, delta).

## Produces
Implements: `ControlStripInterface`

## Consumes
ColoursDefinition
RotaryKnobInterface
AlgorithmSelectorInterface

## Relevant Files
Create: `M-LIM/src/ui/ControlStrip.h` — class declaration
Create: `M-LIM/src/ui/ControlStrip.cpp` — implementation
Read: `M-LIM/src/ui/RotaryKnob.h` — knob component
Read: `M-LIM/src/ui/AlgorithmSelector.h` — algorithm selector
Read: `M-LIM/src/ui/Colours.h` — color constants
Read: `SPEC.md` — ControlStripInterface

## Acceptance Criteria
- [ ] Run: `cd M-LIM && cmake --build build --target MLIM_VST3 -j$(nproc) 2>&1 | tail -3` → Expected: builds without errors

## Tests
None (visual component)

## Technical Details
- Layout: two rows
  - Top row (knobs): AlgorithmSelector | Lookahead knob | Attack knob | Release knob | Channel Link Transients knob | Channel Link Release knob
  - Bottom row (toggles): Gain readout | TP toggle | Oversampling dropdown (Off/2x/4x/8x/16x/32x) | Dither dropdown | DC toggle | Bypass button | Unity (1:1) button | Delta button
- All knobs use RotaryKnob component, attached to APVTS parameters
- AlgorithmSelector attached to "algorithm" parameter
- Toggle buttons: small, dark background, lit when active (accent blue glow)
- Oversampling dropdown: ComboBox attached to "oversamplingFactor" parameter
- Dither section: enable toggle + bit depth dropdown + noise shaping dropdown
- Input gain knob or slider on far left
- Output ceiling knob or slider on far right
- All APVTS attachments created and stored as members

## Dependencies
Requires tasks 018, 019
