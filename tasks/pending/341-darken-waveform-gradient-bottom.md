# Task 341: Darken Waveform Gradient Bottom Color

## Description
The waveform display background gradient bottom color is too light compared to the reference.

**Measured discrepancy (VisualParityAuditor, 2026-03-26):**
- Current `displayGradientBottom`: `#687090` (104, 112, 144)
- Reference bottom (from v1-0005.png clean background samples at ~90% height): `#464B67` to `#4B506C`
- Rendered M-LIM at y=380 in 900x500 crop: `#687090`
- Reference at y=380: `#535258`
- Difference: M-LIM is ~20–25 points too bright in R, G, B channels at the bottom

The reference gradient transitions from `~#848DA5` at the top to `~#464B67` at the bottom.
M-LIM currently goes from `#8992AB` (top, close) to `#687090` (bottom, too light).

**Target value:** `#4A4F6B` (approximately: R=74, G=79, B=107).
This is a mid-point between the reference measurements at ~90% height (v1-0005.png).

## Produces
None

## Consumes
None

## Relevant Files
Modify: `src/ui/Colours.h` — change `displayGradientBottom` value
Read: `src/ui/WaveformDisplay.cpp` — verify gradient is applied in `drawBackground()`

## Acceptance Criteria
- [ ] Run: `cd /workspace/M-LIM && cmake --build build -j$(nproc) 2>&1 | grep -c "error:"` → Expected: `0`
- [ ] Run: build + screenshot + crop 900x500; sample color at x=330, y=380 → Expected: darker than current `#687090`, closer to `#4A4F6B`–`#535268`
- [ ] Run: waveform sub-region RMSE (600x400 at x=150,y=50) → Expected: lower than current 23.76%
- [ ] Visual: gradient smoothly transitions from steel-blue at top to darker blue-gray at bottom, matching reference v1-0005.png

## Tests
None

## Technical Details
In `src/ui/Colours.h`, change:
```cpp
const juce::Colour displayGradientBottom{ 0xff687090 };
```
to:
```cpp
const juce::Colour displayGradientBottom{ 0xff4A4F6B };
```

Note: this affects both the `WaveformDisplay::drawBackground()` gradient AND the
`GainReductionMeter` background (which also uses `displayGradientBottom` to blend
with the adjacent waveform display area — task-338).

Do NOT change `displayGradientTop` (`#8992AB`) — it already closely matches the
reference top value (`#848DA5` measured from v1-0005.png).

## Dependencies
None
