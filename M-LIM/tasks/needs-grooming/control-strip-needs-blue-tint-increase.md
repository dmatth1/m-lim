# Task: Increase Blue Tint in Control Strip Gradient

## Description
The control strip gradient is too neutral gray compared to the Pro-L 2 reference. The reference control strip has a distinct blue-purple tint:

Pixel comparison at Y=490 (center of control strip):
- Current: srgb(71,71,81) — near-equal RGB, very slightly blue
- Reference: srgb(97,107,139) — clearly blue-tinted (blue 139 >> red 97)

The reference control strip has ~50% more blue channel than red, creating a visible blue-purple cast. Our control strip is too neutral/warm. The `controlStripTop` (0xff4D4D56 = 77,77,86) and `controlStripBottom` (0xff383842 = 56,56,66) constants need more blue saturation.

## Produces
None

## Consumes
None

## Relevant Files
Modify: `src/ui/Colours.h` — `controlStripTop` and `controlStripBottom` constants
Read: `src/ui/ControlStrip.cpp` — gradient rendering in paint() method

## Acceptance Criteria
- [ ] Run: `cmake --build build --target MLIM_Standalone -j$(nproc)` → Expected: builds successfully
- [ ] Run: screenshot comparison → Expected: control strip has visible blue-purple tint matching reference

## Tests
None

## Technical Details
- Current `controlStripTop` = 0xff4D4D56 (77,77,86) → target ~0xff4D5570 (77,85,112)
- Current `controlStripBottom` = 0xff383842 (56,56,66) → target ~0xff383E58 (56,62,88)
- The knob area in the reference has a notably bluer appearance than our current neutral gray
- Existing task "darken-control-strip-top-gradient" may conflict — coordinate darkening with blue tint increase
- The reference at the knob level shows ~(97,107,139) which has blue=139 dominating by +42 over red

## Dependencies
None
