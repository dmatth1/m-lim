# Task: Increase GR Meter Idle Brightness and Blue Tint

## Description
The gain reduction meter area in M-LIM appears as dark near-black (#313141) at idle, while the Pro-L 2 reference shows a noticeably brighter blue-gray (#67739A = 103,115,154) in the corresponding region. This is because the reference shows the GR meter/scale area with a structural idle fill similar to the output meter, but the M-LIM GR meter only draws a flat `barTrackBackground` (#2A2838) when no gain reduction is occurring.

The GR meter should display a subtle idle gradient fill (using `meterSafe` or a similar blue-gray color at ~0.4-0.5 alpha) to simulate the appearance of a resting meter with visible bar structure, similar to how the output meter has an idle gradient.

## Produces
None

## Consumes
None

## Relevant Files
Modify: `src/ui/GainReductionMeter.cpp` — add idle structural gradient similar to LevelMeter.cpp's idle fill
Read: `src/ui/LevelMeter.cpp` — reference implementation of idle gradient fill
Read: `src/ui/Colours.h` — available meter colors

## Acceptance Criteria
- [ ] Run: `cmake --build build --target MLIM_Standalone -j$(nproc)` → Expected: builds successfully
- [ ] Run: visual comparison → Expected: GR meter shows visible blue-gray structure at idle instead of near-black

## Tests
None

## Technical Details
- The idle fill should use `meterSafe` (#81828A) at ~0.4-0.5 alpha over the bar track background
- Add segment separators (LED-style) to match the reference's segmented meter appearance
- The fill should be subtle enough to not be confused with actual gain reduction
- Reference pixel at GR meter mid-height: (103,115,154) — bright blue-gray

## Dependencies
None
