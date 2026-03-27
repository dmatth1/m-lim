# Task: Increase Output Meter Idle Blue Fill Alpha

## Description
The output meter (right-side stereo bar) idle state is too dark compared to the Pro-L 2 reference. The reference shows the output meter area with significant blue fill even at idle (pixel samples: reference ~(145,154,181) at mid-bar vs M-LIM ~(53,49,67)). The idle structural gradient in LevelMeter.cpp needs higher alpha values in the lower/mid safe-zone region to produce a brighter steel-blue idle appearance, specifically for the output meter which has `showScale_ = false`.

## Produces
None

## Consumes
None

## Relevant Files
Modify: `src/ui/LevelMeter.cpp` — increase idle gradient alpha values for meterSafe stop (lines 102-121); the bottom alpha is 0.80 which sounds high but renders dark due to the barTrackBackground underneath. Consider raising the safe-zone alpha or adding a brighter base fill.
Read: `src/ui/Colours.h` — barTrackBackground and meterSafe color definitions
Read: `src/ui/PluginEditor.cpp` — output meter layout and dimensions

## Acceptance Criteria
- [ ] Run: `cd /workspace/M-LIM && export CCACHE_DIR=/build-cache/ccache && cmake --build build --target MLIM_Standalone -j$(nproc)` → Expected: builds successfully
- [ ] Run: screenshot and RMSE comparison → Expected: Right panel RMSE improves from baseline 29.59% (or current ~22.82%)

## Tests
None

## Technical Details
- The output meter idle gradient at lines 102-121 of LevelMeter.cpp uses `meterSafe.withAlpha(0.80f)` at the bottom, but the barTrackBackground (0xff2A2838) is very dark, so the composite color ends up at ~(53,49,67)
- Reference output meter idle area is ~(145,154,181) — a bright steel-blue
- Either raise the idle gradient alpha to near-opaque in the safe zone, or change barTrackBackground to be brighter for the output meter specifically
- Be careful not to affect the input meter (left side) adversely — the input meter has `showScale_ = true` and slightly different alpha handling

## Dependencies
None
