# Task: Darken Status Bar / Control Strip Lower Background

## Description
The status bar area at the very bottom of the control strip is too bright compared to Pro-L 2 reference. Pixel comparison: M-LIM status bar renders ~(91,90,103) while reference shows ~(45,40,47). The `controlStripBottom` gradient color (0xff444350 = 68,67,80) combined with the status bar area creates an overall brightness ~45 units above the reference. The control strip bottom gradient and/or status bar background need darkening.

## Produces
None

## Consumes
None

## Relevant Files
Modify: `src/ui/Colours.h` — darken `controlStripBottom` from `0xff444350` to approximately `0xff2D2830`
Modify: `src/ui/ControlStrip.cpp` — check if status bar has separate background fill that also needs darkening
Read: `/reference-docs/reference-screenshots/prol2-main-ui.jpg` — reference status bar is very dark

## Acceptance Criteria
- [ ] Run: `cd /workspace/M-LIM && export CCACHE_DIR=/build-cache/ccache && cmake --build build --target MLIM_Standalone -j$(nproc)` → Expected: builds successfully
- [ ] Run: screenshot and pixel sample at status bar area (~y=475 in editor) → Expected: pixel values closer to (45,40,47) than current (91,90,103)

## Tests
None

## Technical Details
- Current `controlStripBottom`: `0xff444350` = RGB(68,67,80)
- Current `controlStripTop`: `0xff5D5D6A` = RGB(93,93,106)
- The rendered status bar at ~(91,90,103) is above both gradient endpoints, suggesting additional fill or the gradient isn't reaching the bottom properly
- Reference status bar at ~(45,40,47) is much darker — nearly half the brightness
- The control strip gradient from task-427 may have been tuned for the knob row but left the status bar too bright
- Consider also whether controlStripTop needs proportional darkening to maintain the gradient ratio

## Dependencies
None
