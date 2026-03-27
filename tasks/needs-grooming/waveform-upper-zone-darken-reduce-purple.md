# Task: Waveform Upper Zone Darken and Reduce Purple Cast

## Description
The upper waveform zone (top 16% of the waveform display, y=40-120 in the 900x500 crop) shows a blue-purple cast and is slightly brighter than the reference. Pixel analysis:
- Reference average: `#322B2C` (R=50, G=43, B=44) — warm dark brownish-grey
- M-LIM current: `#3E353D` (R=62, G=53, B=61) — brighter with excessive purple/blue (+17 in B, +12 in R, +10 in G)

The `displayGradientTop` constant is the base, currently `0xff302528` = (48,37,40). Due to the vertical gradient blending with `displayGradientBottom` and partial overlap with upper idle fill passes, the effective upper zone color is too bright and purple.

To fix: darken `displayGradientTop` by approximately 8 units per channel, targeting something like `0xff28201E` = (40,32,30). This reduces the vertical gradient base and should pull the upper zone composite closer to the reference (50,43,44).

Also evaluate whether the upper idle fill (the `inputWaveform.withAlpha(0.0f→0.48f)` tent centered at y=15-55%) contributes excessive blue. If the base gradient darkening alone isn't sufficient, reduce the peak alpha of the upper idle fill from 0.48 to 0.38.

## Produces
None

## Consumes
None

## Relevant Files
Modify: `src/ui/Colours.h` — `displayGradientTop`
Modify: `src/ui/WaveformDisplay.cpp` — upper idle fill alpha values (line ~318: `withAlpha(0.48f)`)

## Acceptance Criteria
- [ ] Run: pixel color check after build: `convert screenshots/task-wave-upper-after.png -crop 540x80+0+40 +repage -resize 1x1! txt:-` → Expected: B channel ≤ 52 (down from 61), composite closer to #322B2C
- [ ] Run full RMSE: `compare -metric RMSE <ref-crop> <mlim-crop> /dev/null 2>&1` → Expected: Wave zone RMSE improves from 17.59%

## Technical Details
In `Colours.h`, change:
```cpp
const juce::Colour displayGradientTop { 0xff302528 };  // current
// try:
const juce::Colour displayGradientTop { 0xff282020 };  // -8 units: (40,32,32)
```

In `WaveformDisplay.cpp` upper idle fill block (~line 318), if gradient top alone insufficient:
```cpp
// change alpha 0.48f → 0.38f for the peak of the upper tent
```

Build only Standalone: `cmake --build build --target MLIM_Standalone -j$(nproc)`.

## Dependencies
None
