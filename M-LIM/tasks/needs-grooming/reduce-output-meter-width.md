# Task: Reduce Output Meter Width to Match Reference Proportions

## Description
The output meter in M-LIM is 100px wide (`kOutputMeterW = 100` in PluginEditor.h), which is significantly wider than the Pro-L 2 reference. In the reference, the output level meter strip is approximately 30-40px wide (two thin vertical bars with a scale), while the remaining right-side space is occupied by the loudness histogram.

The wide output meter contributes heavily to right-panel RMSE (27.37%) because:
1. The 100px width pushes bright gray idle fill into space where the reference shows the darker loudness histogram
2. Pixel comparison: M-LIM output meter at x=830-870 = #8F8F97 (bright gray), reference at same coordinates = #957D43 (amber histogram bars) or #171518 (black)

Reducing `kOutputMeterW` from 100 to approximately 40px would:
- Better match reference proportions
- Give more space to the loudness histogram panel
- Reduce right-panel RMSE significantly

## Produces
None

## Consumes
None

## Relevant Files
Modify: `src/PluginEditor.h` — change `kOutputMeterW` from 100 to ~40
Modify: `src/ui/LevelMeter.cpp` — may need to adjust scale label positioning for narrower width
Read: `src/PluginEditor.cpp` — layout logic using kOutputMeterW

## Acceptance Criteria
- [ ] Run: `cmake --build build --target MLIM_Standalone -j$(nproc)` → Expected: builds successfully
- [ ] Run: visual comparison → Expected: output meter is thinner, loudness panel takes more space, right panel more closely matches reference proportions

## Tests
None

## Technical Details
- Reduce `kOutputMeterW` from 100 to 40
- Increase `kLoudnessPanelW` from 140 to 200 to fill the reclaimed space (total width stays same)
- LevelMeter's `showScale_` may need adjustment for the narrower width — scale labels could be omitted for the output meter if too cramped
- The output meter's `kScaleW` (20px) may need reducing or the scale can be drawn on the loudness panel side

## Dependencies
None
