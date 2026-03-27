# Task: Darken Waveform Upper Gradient Zone

## Description
The waveform display upper zone (top 20%) is too bright compared to the Pro-L 2 reference. Pixel comparison: M-LIM upper area ~(53,46,51) at y=80 vs reference ~(34,32,37). The displayGradientTop color needs to be darker to better match the reference's near-black upper zone.

## Produces
None

## Consumes
None

## Relevant Files
Modify: `src/ui/Colours.h` — displayGradientTop (currently 0xff282020 = 40,32,32) renders as ~(53,46,51) after idle fill compositing. Need to darken further, possibly to ~(20,18,20)
Read: `src/ui/WaveformDisplay.cpp` — upper idle fill layer (15-55% height) adds brightness on top of the gradient

## Acceptance Criteria
- [ ] Run: `cd /workspace/M-LIM && export CCACHE_DIR=/build-cache/ccache && cmake --build build --target MLIM_Standalone -j$(nproc)` → Expected: builds successfully
- [ ] Run: screenshot and RMSE comparison → Expected: Waveform RMSE improves

## Tests
None

## Technical Details
- displayGradientTop is 0xff282020 (40,32,32) but the upper idle fill layer adds inputWaveform at alpha ~0.32 at the midpoint (around y=35% height), which brightens the upper zone
- The reference upper zone is very dark: ~(34,32,37)
- Options: (a) darken displayGradientTop to ~(18,16,18), or (b) reduce the upper idle fill alpha from 0.32 to 0.15, or both
- The upper idle fill at lines 308-325 of WaveformDisplay.cpp covers 15%-55% height with peak alpha 0.32

## Dependencies
None
