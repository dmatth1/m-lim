# Task: LUFS Histogram Panel Background — Brighten for Right Panel RMSE

## Description

The right panel (LUFS histogram + output meter area) has the highest regional RMSE at
22.36% (wave-18 baseline). Analysis of the histogram background region shows M-LIM is
~13 units darker than reference:

Right panel average:
- Reference: `423C42` (R:66, G:60, B:66)
- M-LIM:     `353237` (R:53, G:50, B:55)
- Gap: ~13 units

Current histogram background colors:
- `loudnessHistogramTop    = 0xff2E2A2C`  (R:46, G:42, B:44)
- `loudnessHistogramBottom = 0xff3A3133`  (R:58, G:49, B:51)

Target (add ~13 to each channel):
```cpp
const juce::Colour loudnessHistogramTop    { 0xff3C3839 };  // +14 from 2E2A2C (wave-19)
const juce::Colour loudnessHistogramBottom { 0xff484041 };  // +14 from 3A3133 (wave-19)
```

`0xff3C3839` = R:60, G:56, B:57 — moving toward reference right panel tone.

**Expected improvement:** 0.2–0.4 pp in right panel RMSE.

## Produces
None

## Consumes
None

## Relevant Files
Modify: `src/ui/Colours.h` — `loudnessHistogramTop` and `loudnessHistogramBottom`
Read: `src/ui/LoudnessPanel.cpp` — `paint()` uses these for histogram area gradient

## Acceptance Criteria
- [ ] Run: build + screenshot + full RMSE → Expected: ≤ 20.74% (no regression vs wave-18)
- [ ] Run: right panel pixel sample (crop x=720-860, y=100-300) → Expected: average closer to `423C42`
- [ ] Run: visual check → Expected: histogram background is slightly warmer/lighter, not washed out

## Tests
None

## Technical Details
Build: `cmake --build /tmp/mlim-build --target MLIM_Standalone -j$(nproc)`

Right panel check:
```bash
convert /tmp/mlim.png -crop 140x200+720+100 -scale 1x1! -format '%[hex:u]' info:
# Should be closer to 423C42 than current 353237
```

## Dependencies
None
