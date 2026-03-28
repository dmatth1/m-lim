# Task: Darken Control Strip Top Gradient

## Description
The control strip knob row background is too bright compared to Pro-L 2 reference. Pixel comparison at knob area background: M-LIM renders ~(138,143,173) while reference shows ~(108,118,151). The `controlStripTop` gradient color (0xff5D5D6A = 93,93,106) is producing a composite with the knob row that's ~30 units too bright per channel. Both `controlStripTop` and `controlStripBottom` need proportional darkening to match the reference's darker purple-gray knob background.

## Produces
None

## Consumes
None

## Relevant Files
Modify: `src/ui/Colours.h` — darken `controlStripTop` from `0xff5D5D6A` (93,93,106) to approximately `0xff4A4A58` (74,74,88) and `controlStripBottom` from `0xff444350` (68,67,80) to approximately `0xff353442` (53,52,66)
Read: `src/ui/ControlStrip.cpp` — uses these colors for the vertical gradient fill
Read: `/reference-docs/reference-screenshots/prol2-features.jpg` — reference control strip background

## Acceptance Criteria
- [ ] Run: `cd /workspace/M-LIM && export CCACHE_DIR=/build-cache/ccache && cmake --build build --target MLIM_Standalone -j$(nproc)` → Expected: builds successfully
- [ ] Run: screenshot and pixel sample at knob area background → Expected: background closer to (108,118,151) than current (138,143,173)

## Tests
None

## Technical Details
- Current `controlStripTop`: `0xff5D5D6A` = RGB(93,93,106) — task-427 brightened this by +8
- Current `controlStripBottom`: `0xff444350` = RGB(68,67,80) — also brightened by +8 in task-427
- The knob faces sit on top of this gradient; darker background improves contrast with the lighter knob faces
- Note: the darken-status-bar-background task (separate) addresses the bottom portion; coordinate the two changes so the gradient remains smooth
- The reference background has more blue saturation than ours; consider increasing B channel proportionally more

## Dependencies
None
