# Task 331: Darken Loudness Panel Background to Match Reference

## Description
The right panel (LoudnessPanel) has been stuck at 29.66% RMSE. One fixable static cause:
the panel background color is too light compared to the reference.

**Pixel measurements from `/reference-docs/reference-screenshots/prol2-main-ui.jpg` in the
loudness panel static background areas (x=1560-1640, at y positions with no histogram bars):**
```
y=150: RGB(38, 33, 39) = #262127
y=200: RGB(33, 31, 36) = #211F24
y=300: RGB(26, 24, 29) = #1A181D
```

**Current value:**
```cpp
const juce::Colour loudnessPanelBackground { 0xff2B2729 };  // RGB(43, 39, 41)
```

This is 10-17 units brighter than the reference (43 vs 33-38). The reference panel background
is approximately `#1E1C21` averaged across the histogram area.

Additionally, the histogram background within the panel (drawn by `drawHistogram`) uses this
same `loudnessPanelBackground` color (from the panel `paint()` call that fills the whole component
before `drawHistogram` overlays). Making it darker will bring the static histogram area closer
to the reference.

## Produces
None

## Consumes
None

## Relevant Files
Modify: `src/ui/Colours.h` — change `loudnessPanelBackground` to `0xff1E1C21`
Read: `src/ui/LoudnessPanel.cpp` — verify panel uses `loudnessPanelBackground` as fill

## Acceptance Criteria
- [ ] Run: `grep "loudnessPanelBackground" /workspace/M-LIM/src/ui/Colours.h` → Expected: contains `0xff1E1C21`
- [ ] Run: `cd /workspace/M-LIM && cmake --build build -j$(nproc) 2>&1 | grep -c "error:"` → Expected: `0`
- [ ] Build, screenshot — Expected: right panel visually darker, closer to reference dark purple-gray

## Tests
None

## Technical Details
In `src/ui/Colours.h`, change:

```cpp
// BEFORE:
const juce::Colour loudnessPanelBackground { 0xff2B2729 };  // medium dark purple-gray, matches Pro-L 2 loudness panel

// AFTER:
const juce::Colour loudnessPanelBackground { 0xff1E1C21 };  // dark purple-gray, matches Pro-L 2 ref (#262127–#1A181D range)
```

Note: The `histogramHighlight` color (used for the target level row background within the histogram)
is currently `0xff2A2A3A`. Keep it as-is — it should remain slightly brighter than the panel
background to maintain the highlight contrast.

## Dependencies
None
