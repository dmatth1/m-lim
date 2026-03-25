# Task 060: Advanced Expandable Panel for Channel Linking

## Description
In Pro-L 2, the control strip has an "ADVANCED >>" toggle button that expands a panel revealing additional Channel Linking controls (Transients and Release knobs). When collapsed, only the basic knobs (Style, Lookahead, Attack, Release) are visible. Task 025 (ControlStrip) lists channel linking knobs as always-visible in the top row, but Pro-L 2 hides them behind the expandable ADVANCED section.

Reference: See `/reference-docs/reference-screenshots/prol2-main-ui.jpg` (showing "ADVANCED" text with ">>" on right side of knob row), `/reference-docs/video-frames/v1-0020.png` and `v2-0080.png` (showing CHANNEL LINKING section with ADVANCED toggle).

## Produces
None

## Consumes
ControlStripInterface
ColoursDefinition

## Relevant Files
Modify: `M-LIM/src/ui/ControlStrip.h` — add collapsed/expanded state, ADVANCED toggle button
Modify: `M-LIM/src/ui/ControlStrip.cpp` — implement expandable layout, show/hide channel linking knobs
Read: `M-LIM/src/ui/Colours.h` — color constants
Read: `/reference-docs/reference-screenshots/prol2-main-ui.jpg` — reference layout

## Acceptance Criteria
- [ ] Run: `cd M-LIM && cmake --build build --target MLIM_VST3 -j$(nproc) 2>&1 | tail -3` → Expected: builds successfully
- [ ] Run: `grep -c "advanced\|Advanced\|ADVANCED\|expanded" M-LIM/src/ui/ControlStrip.cpp` → Expected: at least 3 matches

## Tests
None (visual/interaction component)

## Technical Details
- Add a toggle button labeled "ADVANCED" with ">>" arrow indicator
- Position: right side of the knob row, after the last visible basic knob
- When collapsed (default): show only STYLE, LOOKAHEAD, ATTACK, RELEASE knobs
- When expanded: slide out to reveal CHANNEL LINKING panel with:
  - "CHANNEL LINKING" header text
  - "TRANSIENTS" sub-label + knob (0-100%, default 75%)
  - "RELEASE" sub-label + knob (0-100%, default 100%)
- The expanded panel should have a slightly different background (semi-transparent overlay or rounded rectangle) to visually distinguish it from the main knob row
- Toggle animation: smooth expand/collapse with `juce::ComponentAnimator` or simple show/hide
- ">>" rotates to "<<" or changes style when expanded
- The ADVANCED text should be rendered vertically or as a small button on the right edge

## Dependencies
Requires task 025
