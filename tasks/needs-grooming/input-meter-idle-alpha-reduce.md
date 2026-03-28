# Task: Input Meter Idle Alpha Reduce

## Description
The input level meter (left side, `showScale_ == true`) uses full 1.0x idle alpha, making it very vivid when idle. The Pro-L 2 reference shows the input meter at idle with a more subdued appearance — the meter bars are visible but not at full intensity. Reducing the input meter idle alpha from `1.0f` to `0.65f` will bring it closer to the reference appearance and reduce Left Meters RMSE (currently 24.49%).

## Produces
None

## Consumes
None

## Relevant Files
Modify: `src/ui/LevelMeter.cpp` — line 98: change `1.0f` to `0.65f` for the `showScale_ == true` branch
Modify: `src/ui/LevelMeter.cpp` — lines 238-239: reduce background alpha for showScale from `0.55f/0.65f` to `0.40f/0.50f`

## Acceptance Criteria
- [ ] Run: `grep -n 'aScale.*showScale' src/ui/LevelMeter.cpp` → Expected: shows `0.65f` for showScale branch and `0.35f` for non-showScale branch
- [ ] Run: Build standalone, launch headless, capture screenshot → Expected: input meter bars visible but less vivid

## Tests
None

## Technical Details
In `LevelMeter.cpp` line 98:
```cpp
// Change from:
const float aScale = showScale_ ? 1.0f : 0.18f;
// To:
const float aScale = showScale_ ? 0.65f : 0.35f;
```

This tones down the input meter's idle gradient to match the reference's more muted idle appearance while keeping the bars clearly visible.

## Dependencies
None — can be combined with the output-meter-idle-alpha-increase task since they modify the same line
