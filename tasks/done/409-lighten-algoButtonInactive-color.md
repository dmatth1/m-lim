# Task 409: Lighten algoButtonInactive to Better Match Reference Style Selector Area

## Description

Pixel analysis (wave22 audit) shows the STYLE button area averages:
- M-LIM: (52, 54, 62)
- Reference: (84, 84, 99)

M-LIM algo buttons are ~32 units too dark. The reference Pro-L 2 uses a single large algorithm
selector button showing much more light area than M-LIM's 8-button grid. Lightening
`algoButtonInactive` closes some of the gap.

Current: `0xff3C4455` (R:60, G:68, B:85)
Target area average: (84, 84, 99)
Recommended: `0xff545870` as first try (+24R, +8G, -15B — less blue to warm up)

## Produces
None

## Consumes
None

## Relevant Files
Modify: `src/ui/Colours.h` — change `algoButtonInactive` constant
Read: `src/ui/AlgorithmSelector.cpp` — verify how button colors are applied

## Acceptance Criteria
- [ ] Run: `cmake --build build --target MLIM_Standalone_Standalone -j$(nproc)` → Expected: exit 0
- [ ] Run: crop 170x80+0+407 from 900x500 M-LIM screenshot → Expected: avg closer to (84,84,99) than current (52,54,62)
- [ ] Run: full RMSE compare → Expected: Control RMSE does not worsen vs 21.08% baseline
- [ ] Run: `ls screenshots/task-409-rmse-results.txt` → Expected: file exists

## Tests
None

## Technical Details

- Baseline (wave 22): Control=21.08%
- The algo button area measurement: crop 170x80+0+407 from the 900x500 M-LIM crop
- Keep `algoButtonSelected` (0xff2E3E58) unchanged — only inactive button color
- Target button background in range 0xff4A5060 – 0xff606878
- Also try: `0xff505868` (+20R, +8G, -17B)

**RMSE methodology**: see task 403 for commands.

## Dependencies
None
