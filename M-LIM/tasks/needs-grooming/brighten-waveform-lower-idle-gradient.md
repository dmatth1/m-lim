# Task: Brighten Waveform Lower Idle Gradient

## Description
The waveform display's lower region (bottom 40%) is too dark and desaturated compared to the Pro-L 2 reference. Pixel comparison: M-LIM lower zone ~(120,126,156) at y=300 vs reference ~(144,155,181). The displayGradientBottom color and the idle fill layers in WaveformDisplay.cpp need adjustment to produce a brighter, more blue-saturated lower zone.

## Produces
None

## Consumes
None

## Relevant Files
Modify: `src/ui/WaveformDisplay.cpp` — idle fill layers (lines 295-393), specifically the lower fill alpha (waveformIdleLowFill) and mid-zone fill alpha (waveformIdleMidFill)
Modify: `src/ui/Colours.h` — displayGradientBottom (currently 0xff9E9EC4 = 158,158,196) may need to shift toward ~(170,170,210) for brighter blue; waveformIdleLowFill and waveformIdleMidFill may also need brightening
Read: `/reference-docs/reference-screenshots/prol2-main-ui.jpg` — compare waveform display area

## Acceptance Criteria
- [ ] Run: `cd /workspace/M-LIM && export CCACHE_DIR=/build-cache/ccache && cmake --build build --target MLIM_Standalone -j$(nproc)` → Expected: builds successfully
- [ ] Run: screenshot and RMSE comparison → Expected: Waveform RMSE improves from baseline 20.55% (or current ~15.53%)

## Tests
None

## Technical Details
- The idle fills use inputWaveform, waveformIdleMidFill, and waveformIdleLowFill colors at various alpha levels
- Lower idle fill (62-100% height): gradient alpha 0→0.35 using inputWaveform
- Mid-zone fill (36-82% height): peak alpha 0.52 using waveformIdleMidFill
- The composite of gradient + idle fills needs to be brighter in the lower region
- Increasing waveformIdleLowFill brightness or its alpha, or brightening displayGradientBottom, should help
- Reference lower zone is approximately #909BB5 (144,155,181)

## Dependencies
None
