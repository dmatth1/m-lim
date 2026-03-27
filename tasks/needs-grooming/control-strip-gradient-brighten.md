# Task: Control Strip Gradient Lighten

## Description
The control strip background (y=420-490 in the 900x500 crop) is slightly darker than the reference. Pixel analysis:
- Reference average: `#535564` (R=83, G=85, B=100)
- M-LIM current: `#4C4E59` (R=76, G=78, B=89)
- Gap: M-LIM is ~7-11 units darker

The control strip gradient colors `controlStripTop` and `controlStripBottom` in `Colours.h` both need to be slightly brightened:
- `controlStripTop`: `0xff555561` = (85,85,97) — already close, minor bump needed
- `controlStripBottom`: `0xff3C3B47` = (60,59,71) — needs ~8 units brightening

Target for composite: approach (83,85,100). Since controlStripTop is already at (85,85,97) and the gradient blends to the bottom, brightening controlStripBottom should raise the average.

## Produces
None

## Consumes
None

## Relevant Files
Modify: `src/ui/Colours.h` — `controlStripTop`, `controlStripBottom`

## Acceptance Criteria
- [ ] Run pixel check: `convert screenshots/task-control-strip-after.png -crop 500x70+0+420 +repage -resize 1x1! txt:-` → Expected: R channel ≥ 80 (up from 76), composite closer to #535564
- [ ] Run RMSE: compare -metric RMSE result → Expected: Control zone RMSE improves from 18.84%

## Technical Details
In `Colours.h`:
```cpp
// Current:
const juce::Colour controlStripTop    { 0xff555561 };
const juce::Colour controlStripBottom { 0xff3C3B47 };
// Try:
const juce::Colour controlStripTop    { 0xff5D5D6A };  // +8 units
const juce::Colour controlStripBottom { 0xff444350 };  // +8 units
```

Adjust incrementally to hit target composite ~(83,85,100). Build Standalone: `cmake --build build --target MLIM_Standalone -j$(nproc)`.

## Dependencies
None
