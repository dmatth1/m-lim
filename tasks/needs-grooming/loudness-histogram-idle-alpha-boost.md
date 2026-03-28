# Task: Loudness Histogram Idle Fill Alpha Boost

## Description
The idle warm-fill simulation in the loudness histogram is too faint. The current gaussian peak alpha is `0.28f` with a skip threshold of `0.02f`, making the idle bars barely visible. The Pro-L 2 reference shows clearly visible warm amber histogram bars centered around the target LUFS level even when idle. Increasing the alpha multiplier to `0.45f` will make these bars visible and improve the Right Panel RMSE.

## Produces
None

## Consumes
None

## Relevant Files
Modify: `src/ui/LoudnessPanel.cpp` — lines 369-371: increase gaussian alpha multiplier from `0.28f` to `0.45f`
Read: `src/ui/Colours.h` — `loudnessHistogramIdleFill` colour constant

## Acceptance Criteria
- [ ] Run: `grep -n '0.45f' src/ui/LoudnessPanel.cpp` → Expected: shows the updated alpha multiplier in the idle fill section
- [ ] Run: Build standalone, launch headless, capture screenshot → Expected: histogram area shows visible warm amber bars at idle

## Tests
None

## Technical Details
In `LoudnessPanel.cpp` around line 369:
```cpp
// Change from:
const float alpha = gaussian * 0.28f;
// To:
const float alpha = gaussian * 0.45f;
```

Also widen the bar coverage from `0.85f` to `0.92f` on line 375 to better fill the histogram area.

## Dependencies
None
