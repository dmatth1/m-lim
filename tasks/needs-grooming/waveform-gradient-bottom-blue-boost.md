# Task: Waveform Gradient Bottom — Increase Blue Channel for Better Mid-Zone Match

## Description

The `displayGradientBottom` colour sets the lower end of the waveform background
vertical gradient. Currently `0xff606898`. Pixel analysis shows the reference has
significantly more blue-gray brightness in the lower-mid zone (56-75% height).

**Current vs target:**
- `displayGradientBottom` = `0xff606898` (R=96, G=104, B=152)
- Reference mid-lower zone samples show ~`#7A88B8` equivalent contribution
- Increasing bottom to `0xff708AB4` (R=112, G=138, B=180) adds ~16-28 counts of brightness
  in the lower gradient region

This task is lower risk than the mid-zone boost shift. It directly increases the base
gradient luminance across the entire lower half, complementing the tent fills.

**Verification pixels (idle, no audio):**

| Region | Before (expected) | After (target) |
|--------|-------------------|----------------|
| y=400, x=320 | ~`#67759E` | ~`#727FB0` |
| y=300, x=320 | ~`#5F688C` | ~`#6975A0` |

## Produces
None

## Consumes
None

## Relevant Files
Modify: `src/ui/Colours.h` — `displayGradientBottom` constant (line 23)

## Acceptance Criteria
- [ ] Run: build → Expected: compiles clean
- [ ] Run: compare -metric RMSE wave region → Expected: ≤ 17.00% (improvement from 17.29%)
- [ ] Run: compare -metric RMSE full → Expected: ≤ 19.60% (no regression from 19.82%)
- [ ] Save results to `screenshots/task-NNN-rmse-results.txt`

## Tests
None

## Technical Details

In `Colours.h`, change:
```cpp
// BEFORE:
const juce::Colour displayGradientBottom{ 0xff606898 };  // brightened from 506090 (task-389)

// AFTER:
const juce::Colour displayGradientBottom{ 0xff708AB4 };  // +16R +22G +28B for better lower-mid match
```

If this increases wave RMSE (unlikely but possible if the bottom gets too light relative to
reference's darker waveform fill zone), try a smaller step: `0xff687EA8`.

## Dependencies
None
