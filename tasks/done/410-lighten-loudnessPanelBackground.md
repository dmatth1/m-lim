# Task 410: Lighten loudnessPanelBackground to Match Reference Right Panel Brightness

## Description

Pixel analysis (wave22 audit) shows the GR/loudness panel area averages:
- M-LIM: (52, 46, 47)
- Reference: (79, 76, 91)

M-LIM panel is ~27-44 units too dark across all channels. Increasing `loudnessPanelBackground`
brightness should improve Right RMSE from 23.95%.

Current: `0xff2B2729` (R:43, G:39, B:41)
Reference composite: (79, 76, 91) — includes active meter activity in the reference.
Target: approximately `0xff464356` (+27R, +28G, +21B from current) as first candidate.

## Produces
None

## Consumes
None

## Relevant Files
Modify: `src/ui/Colours.h` — change `loudnessPanelBackground` constant
Read: `src/ui/LoudnessPanel.cpp` — verify where `loudnessPanelBackground` is applied
Read: `src/ui/GainReductionMeter.cpp` — verify GR meter background rendering

## Acceptance Criteria
- [ ] Run: `cmake --build build --target MLIM_Standalone_Standalone -j$(nproc)` → Expected: exit 0
- [ ] Run: launch app on Xvfb :99, crop 190x400+660+0 from M-LIM 900x500 crop → Expected: avg color closer to (79,76,91) than current (52,46,47)
- [ ] Run: full RMSE compare → Expected: Right RMSE improves vs 23.95% baseline
- [ ] Run: `ls screenshots/task-410-rmse-results.txt` → Expected: file exists

## Tests
None

## Technical Details

- Baseline (wave 22): Right=23.95%
- The measurement region (crop 60x350+670+30 of 900x500) covers GR meter + loudness panel
- Reference right panel has active audio metering contributing warm colors; compensate by not
  over-brightening to avoid overshooting
- Keep panel background in the 0xff404050 – 0xff545465 range
- Do NOT change `loudnessHistogramTop/Bottom` in this task — separate concern
- Also try: `0xff4A4858` as alternative candidate

**RMSE methodology**: see task 403 for commands.

## Dependencies
None
