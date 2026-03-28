# Task 536: Left Meters Visual Parity (GR Meter + Input Meter Area)

## Description
The Left subregion (80x500+640+0 in the 900x500 crop) has 25.85% RMSE — the second worst. This region covers the boundary between the waveform/GR meter and the start of the output area. Key differences:

1. **GR meter width/styling**: Pro-L 2's gain reduction indicator is subtle and integrated with the waveform boundary. M-LIM has a separate 12px GR meter strip — compare whether the width, background, and bar coloring match.
2. **Border/separator styling**: Pro-L 2 has a clean transition between the waveform area and the right-side meters. Check if M-LIM's separator lines or gaps match.
3. **Input meter coloring at left edge**: The "Left" crop region also catches any style mismatch in the input meter colors and background.

Compare the 80px strip at x=640 of the 900x500 RMSE crop against the reference. Adjust widths, colors, and borders to improve the match.

## Produces
None

## Consumes
None

## Relevant Files
Modify: `M-LIM/src/ui/GainReductionMeter.cpp` — adjust bar width, colors, background
Modify: `M-LIM/src/ui/LevelMeter.cpp` — adjust input meter segment colors if needed
Modify: `M-LIM/src/PluginEditor.h` — adjust `kGRMeterW` width if needed
Read: `/reference-docs/reference-screenshots/prol2-main-ui.jpg` — reference

## Acceptance Criteria
- [ ] Run: `cmake --build build --target MLIM_Standalone_Standalone -j$(nproc)` → Expected: build succeeds
- [ ] Run: RMSE comparison of Left subregion (80x500+640+0) → Expected: measurable improvement over 25.85% baseline

## Tests
None (visual parity task)

## Technical Details
Build only standalone for testing. Use the RMSE methodology to measure improvement.

## Dependencies
None
