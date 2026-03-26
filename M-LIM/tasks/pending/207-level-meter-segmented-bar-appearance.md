# Task 207: Level Meter — Add Fine Horizontal Segment Lines (Segmented VU Look)

## Description
The current LevelMeter renders smooth filled rectangles for the level bars. The FabFilter Pro-L 2 reference shows **segmented bar meters** with fine horizontal separator lines creating a VU-segment appearance — many thin horizontal gaps evenly spaced across the full height of each bar.

### Reference appearance (prol2-main-ui.jpg, prol2-metering.jpg):
- Level bars divided into fine horizontal segments (~2–3px filled, ~1px gap)
- Segment lines are dark (matching track background)
- The segmented pattern applies to both the filled level portion and the track background
- Creates a "stacked bar" visual rhythm matching professional hardware meters

### Required Change:
In `LevelMeter.cpp` → `drawChannel()`:
After drawing the filled level portion and the track, overlay horizontal segment separator lines across the full bar height at a regular interval (e.g. every 3px).

Draw the separator lines using the bar track background colour to create visual separation between segments.

## Produces
None

## Consumes
None

## Relevant Files
Modify: `src/ui/LevelMeter.cpp` — add segment line overlay in drawChannel()
Read: `src/ui/Colours.h` — barTrackBackground colour constant
Read: `/reference-docs/reference-screenshots/prol2-metering.jpg` — close-up of segmented meter

## Acceptance Criteria
- [ ] Run: `cmake --build build --target MLIM_Standalone -j$(nproc) 2>&1 | tail -3` → Expected: `[100%] Built target MLIM_Standalone`
- [ ] Run: visual inspection — input and output level meters show clearly segmented horizontal banding across the full bar height

## Tests
None

## Technical Details
```cpp
// After drawing filled portion, overlay segment separators:
const float segmentH = 3.0f;   // visible segment height (filled + gap)
const float gapH     = 1.0f;   // gap between segments
const float stride   = segmentH + gapH;
g.setColour(MLIMColours::barTrackBackground);
for (float y = bar.getY(); y < bar.getBottom(); y += stride)
    g.fillRect(bar.getX(), y + segmentH, bar.getWidth(), gapH);
```
- Apply this loop to the `bar` rectangle (full bar area) AFTER all fills and BEFORE peak hold line
- The gapH lines drawn over the filled area will "punch through" creating segment separation

## Dependencies
None
