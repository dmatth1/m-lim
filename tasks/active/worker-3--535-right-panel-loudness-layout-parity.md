# Task 535: Right Panel Loudness Layout and Styling Parity

## Description
The right panel (loudness panel + output meter) has 26.44% RMSE vs the Pro-L 2 reference — the worst subregion. Key differences visible in comparison:

1. **Output meter segment coloring**: Pro-L 2 has vivid green-to-yellow-to-red gradient segments with clear brightness. M-LIM's output meter idle fill is less vivid.
2. **Loudness panel background**: Pro-L 2's right panel background is a slightly warmer dark gray than the waveform area. M-LIM may not match this.
3. **dB scale labels on output meter**: Pro-L 2 shows scale labels (-5, -10, -15, etc.) next to the output meter in a compact font. Compare M-LIM's label placement and font size.
4. **LUFS large readout**: Pro-L 2 shows "-13.2 LUFS" in a large font at the bottom of the loudness panel. Verify M-LIM's large readout matches the sizing and color.
5. **Readout row styling**: Pro-L 2's readout rows (Momentary, Short-Term, etc.) use a smaller, more subdued font. Compare sizing.

Compare the right 180px of the 900x500 RMSE crop against the reference. Adjust colors, font sizes, spacing, and backgrounds to improve visual match.

## Produces
None

## Consumes
None

## Relevant Files
Modify: `M-LIM/src/ui/LoudnessPanel.cpp` — adjust readout font sizes, row spacing, background colors
Modify: `M-LIM/src/ui/LevelMeter.cpp` — adjust output meter segment colors and idle fill
Modify: `M-LIM/src/ui/Colours.h` — update color constants if needed
Read: `/reference-docs/reference-screenshots/prol2-main-ui.jpg` — reference for comparison

## Acceptance Criteria
- [ ] Run: `cmake --build build --target MLIM_Standalone_Standalone -j$(nproc)` → Expected: build succeeds
- [ ] Run: RMSE comparison of Right subregion (180x500+720+0) → Expected: measurable improvement over 26.44% baseline

## Tests
None (visual parity task)

## Technical Details
Use the RMSE methodology from `screenshots/wave23-rmse-results.txt`. Capture before/after screenshots. Focus on the Right subregion specifically. The goal is to reduce the 26.44% RMSE by addressing the most visible differences.

Build only standalone: `cmake --build build --target MLIM_Standalone_Standalone -j$(nproc)`
Screenshot workflow: `source Scripts/ui-test-helper.sh && start_app && screenshot "task-535-before.png" && stop_app`

## Dependencies
None
