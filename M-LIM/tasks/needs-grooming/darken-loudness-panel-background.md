# Task: Darken Loudness Panel Background to Match Reference

## Description
The loudness panel background (readout area below the histogram) is too bright compared to Pro-L 2 reference. Pixel comparison: M-LIM renders ~(54,50,68) while reference shows ~(36,30,34). The `loudnessPanelBackground` constant in Colours.h is `0xff363244` = RGB(54,50,68), which needs to darken to approximately `0xff241E22` = RGB(36,30,34) to match the reference's nearly-black right panel.

## Produces
None

## Consumes
None

## Relevant Files
Modify: `src/ui/Colours.h` — change `loudnessPanelBackground` from `0xff363244` to approximately `0xff241E22`
Read: `src/ui/LoudnessPanel.cpp` — verify this constant is used for the readout area background
Read: `/reference-docs/reference-screenshots/prol2-main-ui.jpg` — reference shows very dark right panel background

## Acceptance Criteria
- [ ] Run: `cd /workspace/M-LIM && export CCACHE_DIR=/build-cache/ccache && cmake --build build --target MLIM_Standalone -j$(nproc)` → Expected: builds successfully
- [ ] Run: screenshot and pixel sample at loudness readout area → Expected: pixel values closer to (36,30,34) than current (54,50,68)

## Tests
None

## Technical Details
- Current value: `0xff363244` = RGB(54,50,68) — too bright by ~18 units per channel
- Target value: approximately `0xff241E22` = RGB(36,30,34) — matches reference dark panel
- The readout rows (Momentary, Short-Term, Integrated, Range, True Peak) and large LUFS display all use this background
- This is a single constant change in Colours.h

## Dependencies
None
