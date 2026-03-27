# Task: Lighten algoButtonInactive to better match reference style selector area

## Description
Pixel analysis (wave22 audit) shows the STYLE button area averages
M-LIM (52,54,62) vs Reference (84,84,99). The M-LIM algo buttons are ~32 units too dark.
The reference Pro-L 2 uses a single large algorithm selector button that shows much more
light area than M-LIM's 8-button grid. Lightening `algoButtonInactive` closes some of the gap.

Current: 0xff3C4455 (R:60, G:68, B:85)
Target area average: (84,84,99) — achieved when button color is brighter
Recommended: 0xff505868 (+20R, +8G, -17B — note: less blue to warm up) 
             or 0xff545870 as first try (+24R, +8G, -15B)

## Produces
None

## Consumes
None

## Relevant Files
Modify: `src/ui/Colours.h` — change `algoButtonInactive` constant
Read: `src/ui/AlgorithmSelector.cpp` — verify how button colors are applied

## Acceptance Criteria
- [ ] Run: `cmake --build build --target MLIM_Standalone -j$(nproc)` → Expected: exit 0
- [ ] Run: crop 170x80+0+407 from 900x500 M-LIM screenshot → Expected: avg closer to (84,84,99) than current (52,54,62)
- [ ] Run: full RMSE compare → Expected: Control RMSE does not worsen vs 21.08% baseline

## Tests
None

## Technical Details
- Baseline (wave 22): Control=21.08%
- The algo button area measurement: crop 170x80+0+407 from the 900x500 M-LIM crop
- Keep `algoButtonSelected` (0xff2E3E58) unchanged — only inactive button color
- The button face color + text averages to the measured value; increasing background
  lightens the average proportionally
- Target button background in range 0xff4A5060 – 0xff606878

## Dependencies
None
