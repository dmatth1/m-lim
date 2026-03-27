# Task: Control Strip Gradient — Brighten by ~18 Units

## Description

Row-by-row pixel analysis of the control strip region (comparison crop y=432–460) shows
M-LIM is consistently ~18–24 units darker than the reference:

| Row | Reference  | M-LIM      | Gap   |
|-----|-----------|------------|-------|
| y=432| 5F5E6C  | 4A4857     | ~21 u |
| y=445| 686C80  | 555462     | ~19 u |
| y=460| 5C6075  | 484554     | ~24 u |

Current values:
- `controlStripTop    = 0xff575468`  (R:87, G:84, B:104)
- `controlStripBottom = 0xff454350`  (R:69, G:67, B:80)

Reference mid-strip average: `686C80` (R:104, G:108, B:128) — ~18 units brighter.

**Fix:** Increase both strip colors by ~18 in each channel:
```cpp
const juce::Colour controlStripTop    { 0xff696578 };  // +18 from 575468 (wave-19)
const juce::Colour controlStripBottom { 0xff565362 };  // +17 from 454350 (wave-19)
```

Note: task 375 already brightened by +13 from an earlier baseline. This is an additional
+18 for the remaining gap.

**Expected improvement:** 0.3–0.5 pp in control strip RMSE, ~0.2 pp full RMSE.

## Produces
None

## Consumes
None

## Relevant Files
Modify: `src/ui/Colours.h` — `controlStripTop` and `controlStripBottom` constants
Read: `src/ui/ControlStrip.cpp` — `paint()` method uses these for gradient fill

## Acceptance Criteria
- [ ] Run: build + screenshot + full RMSE → Expected: ≤ 20.74% (no regression)
- [ ] Run: control strip pixel sample (crop y=445, x=100–500) → Expected: average closer to `686C80`
- [ ] Run: visual check → Expected: control strip area is visibly brighter/lighter without looking washed out

## Tests
None

## Technical Details
Build: `cmake --build /tmp/mlim-build --target MLIM_Standalone -j$(nproc)`

Control strip color check:
```bash
convert /tmp/mlim.png -crop 400x1+100+445 -scale 1x1! -format '%[hex:u]' info:
# Should be closer to 686C80 than current 555462
```

## Dependencies
None
