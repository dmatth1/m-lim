# Task 459: Output Meter dB Scale — Align Labels to Match Pro-L 2

## Description
In Pro-L 2, the dB scale labels on the right side of the output meter show numbers like "-3", "-6", "-9", "-12" etc. positioned to the right of the meter bars. Looking at the M-LIM screenshot vs reference, the scale label positioning and visibility differ:

Pro-L 2 (prol2-main-ui.jpg, v1-0040.png):
- dB labels are small but clearly readable
- Positioned between the meter bars and the loudness panel
- Use a subtle gray color
- Show every 3 dB from 0 to -27

M-LIM's scale labels (in LevelMeter.cpp `drawScale()` lines 191-218):
- 20px wide scale strip on right side
- Labels drawn with `textSecondary` color
- Tick marks at each grid position

The scale is present but the 20px width may be too narrow for clear readability. The labels should also use the "dB" suffix format matching the reference where labels show "-3 dB", "-6 dB" etc. in some views but just numbers in others.

**Fix approach**: Ensure scale labels use consistent formatting matching the waveform grid labels. The 20px width is adequate but verify label text is not being clipped. Add "dB" suffix to match reference consistency.

## Produces
None

## Consumes
None

## Relevant Files
Modify: `src/ui/LevelMeter.cpp` — in `drawScale()`, verify label formatting and add "dB" suffix where the reference shows it
Read: `src/ui/LevelMeter.h` — scale width constant

## Acceptance Criteria
- [ ] Run: `cmake --build build --target MLIM_Standalone -j$(nproc) 2>&1 | tail -3` → Expected: build succeeds

## Tests
None

## Technical Details
The `drawScale()` function draws labels for each `kMeterGridDB` value. Currently formats as just the number (e.g., "-3"). In the Pro-L 2 reference, the scale between meter bars and loudness panel shows just numbers without suffix, but with "dB" suffix on some entries. Match whichever format gives the best RMSE.

## Dependencies
None
