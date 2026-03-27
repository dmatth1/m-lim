# Task: Brighten Input Meter Idle Gradient

## Description
The input meter (left-side bar with scale) idle state is too dark compared to the Pro-L 2 reference. Pixel comparison: M-LIM left panel ~(44,54,60) vs reference ~(106,94,131). The left panel contributes 20.62% RMSE. The idle structural gradient and/or bar track background need to produce a brighter, more purple-tinted idle appearance.

## Produces
None

## Consumes
None

## Relevant Files
Modify: `src/ui/LevelMeter.cpp` — idle gradient section (lines 86-122); the input meter has `showScale_ = true` which uses different alpha tuning (0.55 vs 0.15 for background)
Modify: `src/ui/Colours.h` — barTrackBackground (0xff2A2838) and meterSafe (0xff6879A0) color constants
Read: `src/ui/PluginEditor.cpp` — input meter layout and configuration

## Acceptance Criteria
- [ ] Run: `cd /workspace/M-LIM && export CCACHE_DIR=/build-cache/ccache && cmake --build build --target MLIM_Standalone -j$(nproc)` → Expected: builds successfully
- [ ] Run: screenshot and RMSE comparison → Expected: Left panel RMSE improves from baseline 23.50% (or current ~20.62%)

## Tests
None

## Technical Details
- The input meter area in the reference is ~(106,94,131) — a medium purple-gray — while M-LIM shows ~(44,54,60) — a dark cool gray
- The background gradient for the input meter uses `meterSafe.darker(0.6).withAlpha(0.55f)` at top and `meterSafe.withAlpha(0.65f)` at bottom (lines 241-250 of LevelMeter.cpp, with `showScale_` branch)
- The bar track background is barTrackBackground (0xff2A2838 = 42,40,56)
- To match reference: increase idle gradient alpha or change barTrackBackground to be closer to reference purple
- The reference left panel has visible waveform content (input meter with gain readout) so some brightness is from active audio, but the structural bar track itself should be brighter

## Dependencies
None
