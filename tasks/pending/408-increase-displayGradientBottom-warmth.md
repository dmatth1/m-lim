# Task 408: Increase displayGradientBottom Warmth for Lower Waveform Zone

## Description

Pixel analysis (wave22 audit) shows the lower waveform zone (y=75-95% of display height) averages:
- M-LIM: (103, 119, 154)
- Reference: (132, 138, 166)

M-LIM bottom is ~29 R-units and ~19 G-units too dark/blue. Increasing the red and green channels
in `displayGradientBottom` should close this gap and improve both Wave and Full RMSE.

Current: `0xff708AB4` (R:112, G:138, B:180)
Recommended: `0xff8A96B8` (+26R, +12G, +4B) — warmer while keeping overall blue character.
Try values empirically and pick whichever hits closest to reference lower-zone average (132,138,166).

## Produces
None

## Consumes
None

## Relevant Files
Modify: `src/ui/Colours.h` — change `displayGradientBottom` constant
Read: `src/ui/WaveformDisplay.cpp` — verify gradient rendering direction (top→bottom confirmed)

## Acceptance Criteria
- [ ] Run: `cmake --build build --target MLIM_Standalone_Standalone -j$(nproc)` → Expected: exit 0
- [ ] Run: launch app on Xvfb :99, take screenshot, compute wave RMSE → Expected: Wave RMSE improves vs 19.31% baseline (wave 22)
- [ ] Run: pixel-sample waveform lower zone (crop 650x80+0+300 from M-LIM 900x500 crop) → Expected: avg R closer to 132 (currently 103), G closer to 138 (currently 119)
- [ ] Run: `ls screenshots/task-408-rmse-results.txt` → Expected: file exists

## Tests
None

## Technical Details

- Baseline (wave 22): Full=19.46%, Wave=19.31%, Right=23.95%, Control=21.08%, Left=21.04%
- Gradient goes from `displayGradientTop` at y=area.getY() to `displayGradientBottom` at y=area.getBottom()
- Additional idle fill overlay (inputWaveform, alpha 0.88 at bottom) blends over the gradient
- Mid-zone boost layers cover 36-82% height
- Only change `displayGradientBottom`; leave all other constants unchanged
- Try values in range 0xff808AB2 – 0xff9098B8 iterating +6 to +26 on red channel

**RMSE methodology**: see task 403 for commands.

## Dependencies
None
