# Task: Waveform Display Gradient — Brighten Bottom Color

## Description

The waveform display bottom half (y=330–380 in 900x500 crop) is ~20–30 units too dark
compared to the reference:

| Row | Reference  | M-LIM      | Gap   |
|-----|-----------|------------|-------|
| y=330| 8C92AE  | 6E799A     | ~30 u |
| y=380| 7B819B  | 5F6F98     | ~24 u |

`displayGradientBottom = 0xff506090` = R:80, G:96, B:144.
The reference at y=330 shows `8C92AE` (R:140, G:146, B:174).

Increasing the gradient bottom color will lift the entire lower half of the display.
Combined with the mid-zone fill task, the two together should close most of the gap.

**Fix:**
```cpp
const juce::Colour displayGradientBottom { 0xff606898 };  // brightened from 506090 (wave-19)
```

`0xff606898` = R:96, G:104, B:152 — adds +16 to R, +8 to G, +8 to B,
preserving the blue-dominant character while lifting overall brightness.

If this causes RMSE regression in the bottom zone (y=420 is already close at current),
try `0xff587090` (+8 in R only) as a more conservative alternative.

**Expected improvement:** 0.3–0.6 pp in wave RMSE.

## Produces
None

## Consumes
None

## Relevant Files
Modify: `src/ui/Colours.h` — `displayGradientBottom` constant
Read: `src/ui/WaveformDisplay.cpp` — `drawBackground()` uses this for the background gradient

## Acceptance Criteria
- [ ] Run: build + screenshot + wave RMSE → Expected: wave RMSE decreases from 18.96%
- [ ] Run: full RMSE → Expected: ≤ 20.74% (wave-18 baseline, no regression)
- [ ] Run: pixel at y=330 in comparison crop → Expected: closer to `8C92AE` than current `6E799A`

## Tests
None

## Technical Details
Build: `cmake --build /tmp/mlim-build --target MLIM_Standalone -j$(nproc)`

Check bottom waveform brightness:
```bash
convert /tmp/mlim.png -crop 400x1+100+330 -scale 1x1! -format '%[hex:u]' info:
# Should be closer to 8C92AE than current 6E799A
```

Note: at y=420 the current M-LIM (`535261`) is already SLIGHTLY BRIGHTER than reference
(`4D4B54`), so brightening the bottom gradient could cause a small regression there.
Measure both y=330 and y=420 when validating.

## Dependencies
None
