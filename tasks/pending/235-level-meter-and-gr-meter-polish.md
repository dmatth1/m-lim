# Task 235: Level Meter and GR Meter Visual Polish

## Description
Polish the level meters and gain reduction meter to better match FabFilter Pro-L 2's metering display. Based on reference screenshots (prol2-metering.jpg):

**LevelMeter changes:**
1. Bar color zones: Pro-L 2 uses a solid blue/cyan for normal levels. Verify `meterSafe = 0xff4D88CC` is rendering as expected. If bars appear too narrow, check that the bar width calculation in drawChannel() uses at least 8px per channel.
2. The meter background track should use `barTrackBackground = 0xff222222`.
3. Peak hold line: should be a 2px bright white horizontal line, visible for 2 seconds then decay.
4. Clip indicator: when clipped, the top 3px of the bar area should turn red (meterDanger color).

**GainReductionMeter changes:**
1. The GR bar color should be a clear red (`0xffEE3333`) — not too orange, not too pink.
2. The dB scale on the right of the GR meter should have labels at 0, -3, -6, -9, -12, -18, -24 dB.
3. Peak GR label: display the current peak GR value as a floating text label (e.g., "6.2 dB") in the peak area.
4. The numeric display at the bottom should show in large text (kFontSizeLarge * 2 approximately) with the `textPrimary` color.

Read LevelMeter.cpp and GainReductionMeter.cpp fully before making any changes. Focus on color and size adjustments only.

## Produces
None

## Consumes
None

## Relevant Files
Read: `src/ui/LevelMeter.cpp` — current implementation
Read: `src/ui/GainReductionMeter.cpp` — current implementation
Modify: `src/ui/LevelMeter.cpp` — adjust bar colors and widths if needed
Modify: `src/ui/GainReductionMeter.cpp` — adjust GR bar color and scale labels
Read: `src/ui/Colours.h` — color constants
Read: `/reference-docs/reference-screenshots/prol2-metering.jpg` — metering reference

## Acceptance Criteria
- [ ] Run: `cd /workspace/M-LIM && cmake --build build --config Release -j$(nproc) 2>&1 | tail -5` → Expected: exits 0
- [ ] Run: `grep "EE3333\|ee3333" /workspace/M-LIM/src/ui/GainReductionMeter.cpp` → Expected: at least 1 match OR the gain reduction bar uses a color constant from Colours.h that evaluates to a red variant

## Tests
None

## Technical Details
- Do not change LevelMeter or GainReductionMeter's public API
- If the GR meter bar already uses MLIMColours::gainReduction (0xffFF4444), it's acceptable — just ensure it looks right visually
- The level meter bars should be at least 8px wide per channel when the component is at its default width

## Dependencies
None
