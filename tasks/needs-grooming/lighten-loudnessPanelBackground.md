# Task: Lighten loudnessPanelBackground to match reference right panel brightness

## Description
Pixel analysis (wave22 audit) shows the GR/loudness panel area averages
M-LIM (52,46,47) vs Reference (79,76,91). The M-LIM panel is ~27-44 units too dark
across all channels. Increasing `loudnessPanelBackground` brightness should improve Right RMSE
from 23.95%.

Current: 0xff2B2729 (R:43, G:39, B:41)
Reference composite: (79,76,91) — note this includes active meter activity in the reference.
Target background: approximately 0xff464357 or 0xff4A4858 to account for panel elements.
Try: 0xff464356 as first candidate (+27R, +28G, +21B from current).

## Produces
None

## Consumes
None

## Relevant Files
Modify: `src/ui/Colours.h` — change `loudnessPanelBackground` constant
Read: `src/ui/LoudnessPanel.cpp` — verify where `loudnessPanelBackground` is applied
Read: `src/ui/GainReductionMeter.cpp` — verify GR meter background rendering

## Acceptance Criteria
- [ ] Run: `cmake --build build --target MLIM_Standalone -j$(nproc)` → Expected: exit 0
- [ ] Run: launch app on Xvfb :99, crop 190x400+660+0 from M-LIM 900x500 crop → Expected: avg color closer to (79,76,91) than current (52,46,47)
- [ ] Run: full RMSE compare → Expected: Right RMSE improves vs 23.95% baseline

## Tests
None

## Technical Details
- Baseline (wave 22): Right=23.95%
- The measurement region (crop 60x350+670+30 of 900x500) covers the GR meter + loudness panel area
- Reference right panel has active audio metering which contributes warm colors; compensate by not
  over-brightening to avoid overshooting
- Keep the panel background in the 0xff404050 – 0xff545465 range
- Do NOT change `loudnessHistogramTop/Bottom` in this task — separate concern

## Dependencies
None
