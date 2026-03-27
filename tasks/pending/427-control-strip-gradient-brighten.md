# Task 427: Control Strip Gradient Brighten

## Description
The control strip background (y=420–490 in the 900x500 crop) is ~7–11 units darker
than the reference.

- Reference avg: `#535564` (R=83, G=85, B=100)
- M-LIM current: `#4C4E59` (R=76, G=78, B=89)

Both `controlStripTop` and `controlStripBottom` in `Colours.h` need brightening:
- `controlStripTop`: `0xff555561` (85,85,97) → try `0xff5D5D6A` (+8)
- `controlStripBottom`: `0xff3C3B47` (60,59,71) → try `0xff444350` (+8)

## Produces
None

## Consumes
None

## Relevant Files
Modify: `src/ui/Colours.h` — `controlStripTop`, `controlStripBottom` (~lines 68–69)

## Acceptance Criteria
- [ ] Run: `cmake --build build --target MLIM_Standalone_Standalone -j$(nproc) 2>&1 | tail -3` → Expected: build succeeds, exit 0
- [ ] Run: pixel check `convert screenshot.png -crop 500x70+0+420 +repage -resize 1x1! txt:-` → Expected: R channel ≥ 80 (up from 76)
- [ ] Run: Control zone RMSE → Expected: improvement

## Tests
None

## Technical Details
In `Colours.h`:
```cpp
const juce::Colour controlStripTop    { 0xff5D5D6A };  // +8 units from 0xff555561
const juce::Colour controlStripBottom { 0xff444350 };  // +8 units from 0xff3C3B47
```

Adjust incrementally to hit composite ~(83,85,100). Build Standalone: `cmake --build build --target MLIM_Standalone_Standalone -j$(nproc)`.

## Dependencies
None
