# Task 456: Loudness Panel Histogram — Show Faint Idle Structure

## Description
The right-side loudness panel histogram area (140px wide) currently shows a flat dark gradient in idle state. In the Pro-L 2 reference (prol2-metering.jpg, prol2-goodies.jpg), the histogram area shows a structured grid with visible bin separators even when no audio is playing — it looks like a grid of small rectangles with subtle borders rather than a flat dark area.

RMSE for LUFS readout region: 23.1% — second highest impact area.

The Pro-L 2 histogram shows:
- Horizontal bin rows with visible 1px dark separators between them
- A slightly brighter column grid structure (vertical lines every ~10 LUFS)
- The dB scale labels on the left of the histogram are very small
- Target level row has a subtle highlight

Currently in `LoudnessPanel.cpp`, the histogram draws bins only when they have non-zero counts. In idle state, this means the area is completely flat, which differs significantly from the structured grid appearance in Pro-L 2.

**Fix approach**: Draw faint empty bin outlines/separators in the histogram area even when counts are zero. Add subtle horizontal lines between bin rows (1px, `panelBorder` at 0.3 alpha) and vertical guide lines at major LUFS marks. This creates the structured grid appearance visible in Pro-L 2 even at idle.

## Produces
None

## Consumes
None

## Relevant Files
Modify: `src/ui/LoudnessPanel.cpp` — in the histogram drawing section, add code to draw faint bin separators and vertical guide lines in the histogram area regardless of bin counts
Read: `src/ui/LoudnessPanel.h` — understand histogram bin layout

## Acceptance Criteria
- [ ] Run: `cmake --build build --target MLIM_Standalone -j$(nproc) 2>&1 | tail -3` → Expected: build succeeds
- [ ] Run: `source Scripts/ui-test-helper.sh && start_app && screenshot "histogram-idle.png" && stop_app` → Expected: histogram area shows visible grid structure even with no audio

## Tests
None

## Technical Details
The histogram has 70 bins (-35 to 0 LUFS in 0.5 dB steps). Each bin occupies a horizontal row. Drawing 1px dark lines between rows and 1px vertical lines at -5, -10, -15, -20, -25, -30, -35 LUFS creates the grid appearance.

Reference: `/reference-docs/reference-screenshots/prol2-metering.jpg` shows the structured histogram clearly.

## Dependencies
None
