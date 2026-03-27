# Task: Increase displayGradientBottom warmth for lower waveform zone

## Description
Pixel analysis (wave22 audit) shows the lower waveform zone (y=75-95% of display height) averages
M-LIM (103,119,154) vs Reference (132,138,166). The M-LIM bottom is ~29 R-units, ~19 G-units too
dark/blue. Increasing the red and green channels in `displayGradientBottom` should close this gap
and improve both Wave and Full RMSE.

Recommended starting point: 0xff708AB4 → 0xff8A96B8  (+26R, +12G, +4B)
This makes it warmer (red/green channels closer to blue) while keeping the overall blue character.
Try values empirically and pick whichever hits closest to reference lower-zone average (132,138,166).

## Produces
None

## Consumes
None

## Relevant Files
Modify: `src/ui/Colours.h` — change `displayGradientBottom` constant
Read: `src/ui/WaveformDisplay.cpp` — verify gradient rendering direction (top→bottom confirmed)

## Acceptance Criteria
- [ ] Run: `cmake --build build --target MLIM_Standalone -j$(nproc)` → Expected: exit 0
- [ ] Run: launch app on Xvfb :99, take screenshot, crop 908x500+509+325, resize 900x500!, compute RMSE against reference → Expected: Wave RMSE improves vs 19.31% baseline (wave 22)
- [ ] Run: pixel-sample waveform lower zone (crop 650x80+0+300 from M-LIM 900x500 crop) → Expected: avg R closer to 132 (currently 103), G closer to 138 (currently 119)

## Tests
None

## Technical Details
- Baseline (wave 22): Full=19.46%, Wave=19.31%, Right=23.95%, Control=21.08%, Left=21.04%
- Gradient goes from `displayGradientTop` at y=area.getY() to `displayGradientBottom` at y=area.getBottom()
- Additional idle fill overlay (inputWaveform, alpha 0.88 at bottom) blends over the gradient
- Mid-zone boost layers (0xff828AA5, alpha 0.80/0.65) cover 36-82% height
- Only change `displayGradientBottom`; leave all other constants unchanged
- Try values in range 0xff808AB2 – 0xff9098B8 iterating +6 to +26 on red channel

## Dependencies
None
