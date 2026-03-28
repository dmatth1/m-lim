# Task: Output Meter Idle Alpha Increase

## Description
The output level meter (right side) is far too dark when idle. The current idle alpha scale for output meters (`showScale_ == false`) is `0.18f` (18%), making the bars nearly invisible. The Pro-L 2 reference shows the output meter bars clearly visible at idle with warm yellow/amber segmented bars at roughly 30-40% opacity. Increasing this to `0.35f` will make the segmented gradient visible and significantly reduce Right Panel RMSE (currently 22.71%).

## Produces
None

## Consumes
None

## Relevant Files
Modify: `src/ui/LevelMeter.cpp` — line 98: change `0.18f` to `0.35f` in the `aScale` ternary for `showScale_ == false`
Read: `src/ui/Colours.h` — meter colour constants used in the idle gradient

## Acceptance Criteria
- [ ] Run: `grep -n 'aScale.*showScale' src/ui/LevelMeter.cpp` → Expected: shows `0.35f` for the non-showScale branch
- [ ] Run: Build standalone, launch headless, capture screenshot, crop right panel (180x500+720+0), compare RMSE → Expected: Right Panel RMSE < 21%

## Tests
None

## Technical Details
In `LevelMeter.cpp` line 98:
```cpp
// Change from:
const float aScale = showScale_ ? 1.0f : 0.18f;
// To:
const float aScale = showScale_ ? 1.0f : 0.35f;
```
This single constant controls the opacity of the entire idle gradient for output meters. The reference shows the output meter bars as clearly visible (muted but not invisible) at idle.

Also adjust the background alpha on lines 238-239:
```cpp
// Change from:
const float bgAlphaTop = showScale_ ? 0.55f : 0.08f;
const float bgAlphaBot = showScale_ ? 0.65f : 0.12f;
// To:
const float bgAlphaTop = showScale_ ? 0.55f : 0.16f;
const float bgAlphaBot = showScale_ ? 0.65f : 0.22f;
```

## Dependencies
None
