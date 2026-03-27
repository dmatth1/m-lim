# Task: Add Idle Histogram Bar Fill to Loudness Panel

## Description
The loudness panel histogram area in Pro-L 2 shows visible bar content even at idle/startup — a pattern of dim blue-gray bars across the histogram. In M-LIM, the histogram area is a flat dark background when no audio is playing. Adding a subtle idle histogram bar pattern would improve right-panel RMSE by filling the visually empty space to better match the reference appearance.

## Produces
None

## Consumes
None

## Relevant Files
Modify: `src/ui/LoudnessPanel.cpp` — histogram paint section (lines 329-355); add idle bar fill when histogram data is empty/zero
Read: `src/ui/Colours.h` — meterSafe, loudnessHistogramTop/Bottom colors
Read: `/reference-docs/reference-screenshots/prol2-main-ui.jpg` — reference shows dim bar content in histogram area

## Acceptance Criteria
- [ ] Run: `cd /workspace/M-LIM && export CCACHE_DIR=/build-cache/ccache && cmake --build build --target MLIM_Standalone -j$(nproc)` → Expected: builds successfully
- [ ] Run: screenshot and RMSE comparison → Expected: Right panel RMSE improves from current ~22.82%

## Tests
None

## Technical Details
- The reference histogram area shows a gradient of dim bars — taller toward the center (around -14 LUFS target) and shorter at extremes
- Implementation: in the histogram paint code, when all bins are zero, draw a Gaussian-shaped idle pattern of bars at ~20-40% height centered on the target LUFS
- Use meterSafe.withAlpha(0.3-0.5) for the idle bars
- This is a cosmetic enhancement — the bars should disappear once real audio data arrives

## Dependencies
None
