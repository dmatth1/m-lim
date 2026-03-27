# Task 429: Loudness Panel Background Darken

## Description
The loudness panel background zone (x=660–800 in the 900x500 crop) is ~11–20 units
too bright compared to the reference.

- Reference avg: `#3A353F` (R=58, G=53, B=63)
- M-LIM current: `#454453` (R=69, G=68, B=83)

All three related constants in `Colours.h` need darkening:
- `loudnessPanelBackground`: `0xff464356` (70,67,86) → try `0xff363244` (−16)
- `loudnessHistogramTop`: `0xff484858` (72,72,88) → try `0xff383848` (−16)
- `loudnessHistogramBottom`: `0xff404050` (64,64,80) → try `0xff303040` (−16)

## Produces
None

## Consumes
None

## Relevant Files
Modify: `src/ui/Colours.h` — `loudnessPanelBackground`, `loudnessHistogramTop`, `loudnessHistogramBottom` (~lines 61–65)
Read: `src/ui/LoudnessPanel.cpp` — to understand how background is painted

## Acceptance Criteria
- [ ] Run: `cmake --build build --target MLIM_Standalone_Standalone -j$(nproc) 2>&1 | tail -3` → Expected: build succeeds, exit 0
- [ ] Run: pixel check `convert screenshot.png -crop 140x340+660+40 +repage -resize 1x1! txt:-` → Expected: composite R ≤ 62 (down from 69)
- [ ] Visual: loudness panel visibly darker, closer to reference

## Tests
None

## Technical Details
In `Colours.h`:
```cpp
const juce::Colour loudnessPanelBackground { 0xff363244 };  // was 0xff464356
const juce::Colour loudnessHistogramTop    { 0xff383848 };  // was 0xff484858
const juce::Colour loudnessHistogramBottom { 0xff303040 };  // was 0xff404050
```

Adjust to hit composite ~(58,53,63). Build Standalone: `cmake --build build --target MLIM_Standalone_Standalone -j$(nproc)`.

## Dependencies
None
