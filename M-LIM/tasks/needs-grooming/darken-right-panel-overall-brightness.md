# Task: Darken Right Panel (Loudness + Output Meter) Overall Brightness

## Description
The entire right panel area (loudness histogram + output meter + GR meter) is significantly brighter than the Pro-L 2 reference. The right panel RMSE is 25.9%.

Key pixel comparisons at x=750 (loudness panel area):
- Y=160: current srgb(88,80,77), reference srgb(24,22,25) — 60+ units too bright
- Y=200: current srgb(75,69,76), reference srgb(22,20,23) — 50+ units too bright
- Y=300: current srgb(58,56,74), reference srgb(37,28,31) — 25+ units too bright, also too blue

The reference right panel is predominantly near-black with only the active histogram bars and scale labels providing brightness. Our loudness panel background colors are far too light:
- `loudnessPanelBackground` = 0xff3A384A (58,56,74) — should be ~0xff1E1C22 (30,28,34)
- `loudnessHistogramTop` = 0xff424050 (66,64,80) — should be ~0xff1A1822 (26,24,34)
- `loudnessHistogramBottom` = 0xff3A384A (58,56,74) — should be ~0xff1E1C22 (30,28,34)

Note: There are existing tasks for "darken-loudness-panel-background" and "darken-loudness-histogram-gradient" but those may use insufficient darkening targets. This task specifies the exact reference pixel values to match.

## Produces
None

## Consumes
None

## Relevant Files
Modify: `src/ui/Colours.h` — `loudnessPanelBackground`, `loudnessHistogramTop`, `loudnessHistogramBottom` constants
Modify: `src/ui/LoudnessPanel.cpp` — verify background paint code produces near-black appearance
Read: `src/PluginEditor.h` — right panel layout constants

## Acceptance Criteria
- [ ] Run: `cmake --build build --target MLIM_Standalone -j$(nproc)` → Expected: builds successfully
- [ ] Run: screenshot RMSE comparison → Expected: right panel RMSE improves from ~25.9% toward <18%

## Tests
None

## Technical Details
- Target colors based on reference pixel sampling:
  - `loudnessPanelBackground`: change from 0xff3A384A to ~0xff1E1C24
  - `loudnessHistogramTop`: change from 0xff424050 to ~0xff1C1A24
  - `loudnessHistogramBottom`: change from 0xff3A384A to ~0xff1E1C24
- The `grMeterBackground` (0xff2E2C3A) between waveform and right panel should also be darkened to ~0xff1A1822
- Check that text and histogram bars remain legible against the darker background

## Dependencies
None
