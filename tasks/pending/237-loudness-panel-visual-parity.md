# Task 237: Loudness Panel — LUFS Display Visual Parity

## Description
Refine the LoudnessPanel component to better match FabFilter Pro-L 2's loudness display. Based on reference screenshots (prol2-metering.jpg and prol2-main-ui.jpg):

**Changes needed:**
1. **Large numeric readout**: The LUFS value should be displayed in a very large font (~28-32pt) in the center of the panel. Use `textPrimary` (white/near-white) color. Current implementation may already do this — verify and adjust size if needed.

2. **Histogram bars**: The horizontal LUFS histogram bars (loudness over time) should use `lufsReadoutGood = 0xffE8C040` (warm golden-yellow) when below the target loudness, and `meterAtTarget = 0xffFF8C00` (orange) near the target, and `meterWarning` (yellow) when above target. This creates the warm amber histogram look from the reference.

3. **Target marker**: A clear visual marker (vertical line or highlighted row) at the configured target LUFS level (-14 LUFS default). Use `histogramHighlight = 0xff2A2A3A` for the target row background.

4. **Measurement mode label**: Below the numeric readout, show the current measurement mode in small text ("Momentary", "Short Term", "Integrated"). This should be in `textSecondary` color at kFontSizeSmall.

5. **Panel border**: The loudness panel should have a 1px border using `panelBorder` color, with rounded corners (4px radius).

Read LoudnessPanel.h and LoudnessPanel.cpp fully before making changes.

## Produces
None

## Consumes
None

## Relevant Files
Read: `src/ui/LoudnessPanel.h` — current interface
Read: `src/ui/LoudnessPanel.cpp` — current implementation
Modify: `src/ui/LoudnessPanel.cpp` — adjust rendering for histogram colors and readout size
Read: `src/ui/Colours.h` — color constants (lufsReadoutGood, meterAtTarget, histogramHighlight)
Read: `/reference-docs/reference-screenshots/prol2-metering.jpg` — detailed metering reference

## Acceptance Criteria
- [ ] Run: `cd /workspace/M-LIM && cmake --build build --config Release -j$(nproc) 2>&1 | tail -5` → Expected: exits 0
- [ ] Run: `grep "lufsReadoutGood\|E8C040" /workspace/M-LIM/src/ui/LoudnessPanel.cpp` → Expected: at least 1 match (histogram uses warm yellow)

## Tests
None

## Technical Details
- The numeric readout size: use `juce::Font(28.0f, juce::Font::bold)` for the main LUFS number
- Histogram direction: horizontal bars growing left-to-right, newest at top
- If the panel already has most of this, just verify the colors match and adjust font size

## Dependencies
None
