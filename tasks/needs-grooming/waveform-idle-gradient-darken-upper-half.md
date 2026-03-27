# Task: Waveform Idle Gradient — Darken Upper 40% Further

## Description
Waveform area RMSE is 17.6%. The M-LIM waveform idle state still has too much brightness in the upper 40% (0 to -12 dBFS zone) compared to the Pro-L 2 reference. In the reference, the top portion of the waveform display is very dark (nearly black) because this is the zone above the audio content where gain reduction occurs.

Current state after tasks 421-422:
- `displayGradientTop`: 0xff282020 (dark, good)
- Upper idle fill at 15-55% height with peak alpha 0.32 (task-421 reduced from 0.48)

The upper fill at 0.32 alpha is still contributing too much brightness in the 15-30% height zone. The reference shows this zone should be nearly as dark as the top gradient. The Pro-L 2 waveform has content starting around 30-40% down from the top (around -9 to -12 dBFS), with the area above being very dark.

**Fix approach**: Reduce the upper idle fill peak alpha from 0.32 to 0.20 and narrow the upper fill zone from 15-55% to 20-50%. This darkens the top portion to better match the Pro-L 2 reference where the upper zone is nearly black.

## Produces
None

## Consumes
None

## Relevant Files
Modify: `src/ui/WaveformDisplay.cpp` — in `drawBackground()`, reduce upper idle fill alpha (lines 311-328): change 0.15f/0.55f to 0.20f/0.50f and reduce peak alpha from 0.32f to 0.20f

## Acceptance Criteria
- [ ] Run: `cmake --build build --target MLIM_Standalone -j$(nproc) 2>&1 | tail -3` → Expected: build succeeds
- [ ] Run: `source Scripts/ui-test-helper.sh && start_app && screenshot "waveform-dark.png" && stop_app` → Expected: upper waveform zone is darker

## Tests
None

## Technical Details
The upper idle fill creates a tent-shaped alpha fill between 15-55% of display height (lines 311-328 in WaveformDisplay.cpp). The peak alpha at 30% height is 0.32 (reduced from 0.48 in task-421). Further reducing to 0.20 and narrowing the range will reduce the brightness in the -3 to -9 dB zone where Pro-L 2 shows near-black.

Target: bring waveform RMSE from 17.6% closer to 15%.

## Dependencies
None
