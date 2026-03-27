# Task: Waveform Top Gradient — Darken and Cool Slightly

## Description

Pixel analysis of the waveform upper zone shows M-LIM's top is slightly too warm
and too light compared to the reference:

| Height | M-LIM | Reference | Issue |
|--------|-------|-----------|-------|
| y=50 (10%) | `#342C31` | `#262127` | M-LIM ~14 counts lighter, slightly warmer |
| y=100 (20%) | `#3A343F` | `#201E24` | M-LIM ~25 counts lighter |
| y=150 (30%) | `#464355` | `#1C1A1D` | M-LIM much lighter (reference shows dark waveform fills) |

The top of the waveform in the reference is very dark (the dark waveform fills dominate
the upper zone in the reference screenshot which captured active audio). M-LIM's idle
background is too light at the top because no waveform fills are present.

Darkening `displayGradientTop` slightly (and cooling its hue) would reduce the brightness
mismatch at y=50-100 where the gap is purely gradient-driven (not masked by waveform data).

**Note:** Task-391 shifted this toward warm (`0xff332A2D`). This task partially
reverses that — the warm shift may have overshot for the top zone.

## Produces
None

## Consumes
None

## Relevant Files
Modify: `src/ui/Colours.h` — `displayGradientTop` constant (line 22)

## Acceptance Criteria
- [ ] Run: build → Expected: compiles clean
- [ ] Run: pixel at y=50, x=320 → Expected: ≤ `#2E2830` (darker/cooler than current `#342C31`)
- [ ] Run: compare -metric RMSE wave region → Expected: ≤ wave-20 baseline 17.29%
- [ ] Run: compare -metric RMSE full → Expected: ≤ 19.82% (no regression)
- [ ] Save results to `screenshots/task-NNN-rmse-results.txt`

## Tests
None

## Technical Details

In `Colours.h`, change:
```cpp
// BEFORE:
const juce::Colour displayGradientTop   { 0xff332A2D };  // warm near-black shift toward reference 39332A (task-391)

// AFTER:
const juce::Colour displayGradientTop   { 0xff28242A };  // darken and cool slightly: R-11, G-6, B-3
```

Target: approximately split the difference between current (`#332A2D`) and
reference top zone (`#262127`). The change is `#28242A`:
- R: 0x33→0x28 (-11)
- G: 0x2A→0x24 (-6)
- B: 0x2D→0x2A (-3) — slight cool shift

If this makes the transition too abrupt (noticeable banding), try `#2E2829` as a
smaller intermediate step.

## Dependencies
None
