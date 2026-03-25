# Task 057: Loudness Histogram Meter Display

## Description
Task 023 (LoudnessPanel) describes the loudness panel as a simple "vertical stack of labeled readings" with "small horizontal bars." This drastically underspecifies the actual Pro-L 2 loudness display, which is a **full loudness distribution histogram** — one of the plugin's signature visual features. The right-side loudness meter in Pro-L 2 shows horizontal bars at each dB level representing how much time the audio has spent at that loudness, color-coded from white→yellow→orange→red relative to the target. This task adds the histogram visualization that task 023 omits.

Reference: See `/reference-docs/reference-screenshots/prol2-metering.jpg` and `/reference-docs/reference-screenshots/prol2-main-ui.jpg` (right panel), and `/reference-docs/video-frames/v1-0015.png`, `v1-0025.png`, `v1-0040.png`.

## Produces
None

## Consumes
LoudnessPanelInterface
ColoursDefinition
LoudnessMeterInterface

## Relevant Files
Modify: `M-LIM/src/ui/LoudnessPanel.h` — add histogram data storage and rendering
Modify: `M-LIM/src/ui/LoudnessPanel.cpp` — implement histogram rendering, target indicator, scale
Read: `M-LIM/src/ui/Colours.h` — color constants
Read: `M-LIM/src/dsp/LoudnessMeter.h` — loudness data source
Read: `/reference-docs/reference-screenshots/prol2-metering.jpg` — reference for histogram layout

## Acceptance Criteria
- [ ] Run: `cd M-LIM && cmake --build build --target MLIM_VST3 -j$(nproc) 2>&1 | tail -3` → Expected: builds successfully
- [ ] Run: `grep -c "histogram\|Histogram" M-LIM/src/ui/LoudnessPanel.cpp` → Expected: at least 3 matches
- [ ] Run: `source Scripts/ui-test-helper.sh && start_app && screenshot "screenshots/task-057-after.png" && stop_app && compare_to_reference /reference-docs/reference-screenshots/prol2-main-ui.jpg screenshots/task-057-after.png 0.15` → Expected: visual parity with Pro-L 2's right-side loudness meter area

## Tests
None (visual component — verified by UI parity auditor)

## Technical Details
Pro-L 2's loudness histogram display includes:

1. **Histogram bars**: Horizontal bars at each dB level (approximately 1dB steps from ~-35 to 0 dB). Each bar's width represents accumulated time at that loudness level. Bars are color-coded:
   - White/light gray: normal levels below target
   - Yellow: approaching target
   - Orange: at target level
   - Red: exceeding target level

2. **dB scale**: Vertical scale on the left side of the histogram showing dB markings (0, -3, -6, -9, -12, -15, -18, -21, -24, -27, -30, -33 dB)

3. **Target level indicator**: Highlighted bar/label showing the selected target level (e.g., "-14 (Strm)" with distinct background)

4. **LUFS readout**: Large numeric readout at bottom-right (e.g., "-13.2 LUFS") showing current selected measurement type

5. **Measurement type selector**: "Short Term" / "Momentary" / "Integrated" buttons below the readout

6. **Pause/Reset**: "||" (pause) button and click-to-reset on integrated measurement

7. **Scale selector**: Button showing range mode (e.g., "Strm +9") that opens a menu for EBU +9, EBU +18, or absolute LUFS display

**Implementation approach**:
- Add a `std::array<float, 70>` histogram bin accumulator (covering -35 to 0 dB in 0.5dB steps)
- In `pushLoudnessData()`, increment the bin corresponding to the current short-term/momentary LUFS
- `paint()` draws bars proportional to accumulated counts, normalized to the maximum bin
- Color each bar based on its dB position relative to the target level
- Draw dB scale labels, target indicator, and LUFS readout
- The histogram should continuously accumulate and can be reset with the integrated measurement

## Dependencies
Requires task 023
