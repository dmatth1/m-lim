# Task 522: Waveform Lower Zone Brightness — Reduce inputWaveform Fill to Close Gap

## Description

Pixel measurement shows a brightness gap in the lower waveform zone (y=300 in 900x500 crop):

- M-LIM current: (120, 125, 156) at x=300, y=300
- Reference:     (132, 138, 166) at x=300, y=300
- Gap:           R+12, G+13, B+10 (M-LIM too dark)

Midzone (y=250) is near-perfect and MUST be preserved:
- M-LIM current: (122, 128, 154)
- Reference:     (121, 128, 155)

**Fix** — two-part change in WaveformDisplay.cpp + Colours.h:

1. Reduce `inputWaveform` fill bottom alpha from `0.82f` to `0.70f` (WaveformDisplay.cpp ~line 303)
2. Lighten `waveformIdleLowFill` from `0xff9898A8` to `0xffA8A8B8` in Colours.h, and increase its alpha from `0.35f` to `0.50f` (WaveformDisplay.cpp ~line 393)

## Produces
None

## Consumes
None

## Relevant Files
Modify: `src/ui/WaveformDisplay.cpp` — reduce inputWaveform fill alpha 0.82→0.70, increase lowerFill alpha 0.35→0.50
Modify: `src/ui/Colours.h` — lighten waveformIdleLowFill from 0xff9898A8 to 0xffA8A8B8

## Acceptance Criteria
- [ ] Run: `cmake --build build --target MLIM_Standalone_Standalone -j$(nproc) 2>&1 | tail -3` → Expected: build succeeds
- [ ] Run: Measure pixel at (300,300) in 900x500 crop → Expected: R≥124, G≥128 (closer to reference 132,138,166)
- [ ] Run: Measure pixel at (300,250) in 900x500 crop → Expected: R in 120–126, G in 125–131 (preserve midzone)
- [ ] Run: Wave region RMSE compare → Expected: ≤ 15.89% (no regression from task-498 baseline)

## Tests
None

## Technical Details
- The inputWaveform fill is the large gradient at lines ~295-305 in drawBackground()
- The lowerFill is the small gradient at lines ~383-393 in drawBackground()
- Do NOT modify mid-zone fills or displayGradientBottom

## Dependencies
None
