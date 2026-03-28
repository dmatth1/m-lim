# Task: Darken Status Bar / Control Strip Lower Background

## Description
The status bar area at the very bottom of the control strip is too bright compared to Pro-L 2 reference. Pixel comparison: M-LIM status bar renders ~(72,71,84) while reference shows ~(45,40,47). The control strip bottom gradient and/or status bar background need darkening.

Also, the "True Peak Limiting" toggle button currently renders with a bright green background (#46894C), while in the reference this area appears as a subtler dark element. Consider toning down the green to a dimmer indicator.

## Produces
None

## Consumes
None

## Relevant Files
Modify: `src/ui/Colours.h` — may need additional darkening of `controlStripBottom` (already darkened in sweep-41 from #444350 to #383842)
Modify: `src/ui/ControlStrip.cpp` — check if status bar has separate background fill that also needs darkening; adjust button styling
Read: `src/ui/Colours.h` — check `buttonOnBackground` and `buttonOnText` colors

## Acceptance Criteria
- [ ] Run: `cmake --build build --target MLIM_Standalone -j$(nproc)` → Expected: builds successfully
- [ ] Run: screenshot and pixel sample at status bar area → Expected: pixel values closer to reference (45,40,47)

## Tests
None

## Technical Details
- Current `controlStripBottom` (after sweep-41 fix): #383842 = RGB(56,56,66)
- The rendered status bar at ~(72,71,84) is above the gradient endpoint, suggesting additional fill or the gradient isn't reaching the bottom properly
- Reference status bar at ~(45,40,47) is much darker
- The "True Peak Limiting" on-state should use a dimmer green or subtle teal instead of bright green
- This change will improve control strip RMSE (currently 19.04%)

## Dependencies
None
