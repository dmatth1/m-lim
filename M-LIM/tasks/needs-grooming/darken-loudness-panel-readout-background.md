# Task: Darken Loudness Panel Readout Background

## Description
The loudness panel readout area background (`loudnessPanelBackground` = #363244, RGB 54,50,68) is much brighter than the corresponding region in the Pro-L 2 reference, which measures near-black ~(23,21,24). The readout rows (Momentary, Short-Term, Integrated, Range, True Peak) sit on this bright purple-gray background, contributing significantly to the Right Panel's 27% RMSE.

The `loudnessPanelBackground` color should be darkened from #363244 to approximately #1C1A20 (RGB ~28,26,32) to match the reference's near-black appearance in this region.

Also consider darkening `loudnessHistogramTop` (#383848) and `loudnessHistogramBottom` (#303040) slightly, as the histogram gradient is also brighter than the reference background.

## Produces
None

## Consumes
None

## Relevant Files
Modify: `src/ui/Colours.h` — change `loudnessPanelBackground`, `loudnessHistogramTop`, and `loudnessHistogramBottom` constants (lines 64, 67-68)
Read: `src/ui/LoudnessPanel.cpp` — verify which colors affect which areas

## Acceptance Criteria
- [ ] Run: `cmake --build build --target MLIM_Standalone -j$(nproc)` → Expected: builds successfully
- [ ] Run: visual comparison of right panel region → Expected: readout area is near-black, matching reference darkness level

## Tests
None

## Technical Details
- Suggested values:
  - `loudnessPanelBackground`: #1C1A20 (was #363244) — darkened ~30 units
  - `loudnessHistogramTop`: #242430 (was #383848) — darkened ~20 units
  - `loudnessHistogramBottom`: #1E1E2C (was #303040) — darkened ~18 units
- These changes will significantly reduce Right Panel RMSE (currently highest at 27.37%)

## Dependencies
None
