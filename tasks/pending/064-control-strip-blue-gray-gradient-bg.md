# Task 064: Control Strip Blue-Gray Gradient Background

## Description
Pro-L 2's control strip / knob area has a distinctly blue-gray gradient background that is noticeably different from the waveform display area above. The current colour spec only defines `background` (#1E1E1E neutral dark gray) and `displayBackground` (#141414 near black). Pro-L 2's control area has a medium blue-gray tone creating clear visual separation between the waveform and controls. This is a core visual characteristic of the Pro-L 2 aesthetic.

Reference: See `/reference-docs/reference-screenshots/prol2-main-ui.jpg` (lower half showing blue-gray control area), `/reference-docs/reference-screenshots/prol2-features.jpg` (close-up of knobs on blue-gray background), `/reference-docs/video-frames/v1-0020.png`.

## Produces
None

## Consumes
ColoursDefinition
ControlStripInterface

## Relevant Files
Modify: `M-LIM/src/ui/Colours.h` — add controlStripBackground gradient colors
Modify: `M-LIM/src/ui/ControlStrip.cpp` — paint blue-gray gradient background
Read: `/reference-docs/reference-screenshots/prol2-features.jpg` — reference close-up of control area

## Acceptance Criteria
- [ ] Run: `cd M-LIM && cmake --build build --target MLIM_VST3 -j$(nproc) 2>&1 | tail -3` → Expected: builds successfully
- [ ] Run: `grep "controlStrip\|controlBackground" M-LIM/src/ui/Colours.h` → Expected: at least 1 match

## Tests
None (visual styling)

## Technical Details
- The Pro-L 2 control strip area has a gradient that goes from a slightly lighter blue-gray at the top (roughly #3A3D4A) to a darker blue-gray at the bottom (roughly #2A2D3A)
- This creates clear visual separation from the near-black waveform area above
- Add new color constants to MLIMColours:
  - `controlStripTop` ≈ #3A3D4A (medium blue-gray)
  - `controlStripBottom` ≈ #2A2D3A (darker blue-gray)
- In ControlStrip::paint(), draw a vertical ColourGradient from top to bottom with these colors
- The knobs sit ON TOP of this gradient, so the knob face color (#3A3A3A) naturally blends with the background
- The bottom status bar row may have a slightly darker tint than the knob row
- This blue-gray tint is what makes Pro-L 2's control area feel cohesive and polished — without it, the UI will look flat

## Dependencies
Requires task 003 (Colours.h exists) and task 025 (ControlStrip exists)
