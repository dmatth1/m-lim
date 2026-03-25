# Task 140: Improve Knob Value Arc Visibility

## Description
The rotary knob value arc color `knobArc = 0xff5C6A84` (steel grey-blue) is too close in luminance to the track background `panelBorder = 0xff333333`. The arc is barely distinguishable from the unfilled track arc, making it hard to see the current knob position without careful inspection.

The Pro-L 2 reference (see `/reference-docs/reference-screenshots/prol2-features.jpg`) shows knob arcs that clearly indicate the value position as a visible light-blue accent.

Fix: Increase the brightness/saturation of `knobArc` to approximately `0xff4888C8` (medium blue) so the filled arc is clearly distinguishable from the unfilled track arc at a glance.

## Produces
None

## Consumes
None

## Relevant Files
Modify: `src/ui/Colours.h` — change `knobArc` constant
Read: `src/ui/RotaryKnob.cpp` — understand how `knobArc` is used (track vs value arc), ensure only the value arc uses it

## Acceptance Criteria
- [ ] Run: `source Scripts/ui-test-helper.sh && start_app && screenshot "task-140-after.png" && stop_app` → Expected: screenshot shows knob value arcs as clearly visible blue, distinct from the dark grey unfilled track
- [ ] Run: `cmake --build build -j$(nproc) 2>&1 | grep -c error` → Expected: `0`

## Tests
None

## Technical Details
- `src/ui/Colours.h:29`: `const juce::Colour knobArc { 0xff5C6A84 };` → change to `{ 0xff4888C8 }`
- The track background arc uses `MLIMColours::panelBorder` (line 76 of RotaryKnob.cpp); leave that unchanged
- The value arc fill uses `MLIMColours::knobArc` (line 95 of RotaryKnob.cpp); this is the target

## Dependencies
None
