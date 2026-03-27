# Task: Output Meter Idle Gradient — Simulate Bottom-Up Active-Fill Pattern

## Description
The Right zone RMSE is 26.74% (crop 100x400+800+50 = output level meter at x=800–900).
The output meter at idle shows near-uniform warm amber (~65–71 brightness) throughout.
The reference shows a characteristic "active program at -12 dBFS" pattern: dark near the
top (unfilled track) transitioning to bright yellow at the mid-level fill zone and bright
cool-blue at the bottom of the fill.

Pixel comparison at x=840 (output meter center):
```
y=40  (top):    M-LIM (26,26,26)  | Ref (37,34,39)   — close, both dark
y=100 (~25%):   M-LIM (64,55,42)  | Ref (49,31,37)   — M-LIM too warm/bright
y=200 (~50%):   M-LIM (69,62,48)  | Ref (149,125,67) — Ref VERY BRIGHT (active fill peak)
y=300 (~75%):   M-LIM (71,66,59)  | Ref (62,58,61)   — close; ref slightly darker
y=400 (~100%):  M-LIM (61,64,74)  | Ref (112,118,144) — Ref brighter cool-blue (bottom fill)
```

The reference shows peak brightness (~149,125,67) at y=200 (50% height ≈ -30 dBFS fill)
and bright cool blue (~112,118,144) at y=400 (bottom = -60 dBFS mark). M-LIM is much
dimmer and too warm throughout.

The fix in the companion task (level-meter-idle-gradient-fix-warm-bleed.md) will already
improve this by switching from warm amber to cool blue at the bottom stop. This task
focuses on the additional challenge of matching the brightness at y=200 (~50% fill) by
adding a mid-fill brightness boost in the idle gradient.

## Produces
None

## Consumes
None

## Relevant Files
Modify: `src/ui/LevelMeter.cpp` — `drawChannel()` idle gradient block (lines ~88–117)
Read:   `src/ui/Colours.h` — `meterSafe`, `meterWarning`, `grMeterLow`, `barTrackBackground`

## Acceptance Criteria
- [ ] Run: Right zone RMSE (crop 100x400+800+50) → Expected: Right zone RMSE ≤ 24.00% (improvement from 26.74%)
- [ ] Run: Full RMSE → Expected: Full ≤ 19.50% (no regression from current 19.23%)
- [ ] Run: Left zone RMSE (crop 30x378+0+30) → Expected: ≤ 26.00% (no regression)

## Tests
None

## Technical Details
After applying the warm-bleed fix (separate task), the idle gradient uses cool meterSafe
color bottom-to-top. The remaining gap at y=200 (50% height, ~-30 dBFS) is:
- M-LIM will show ~(74,78,102) there after the warm-bleed fix
- Reference shows (149,125,67) — significantly brighter

To boost brightness at the 40–60% height range (corresponding to -24 to -36 dBFS):

Option: Add a mid-fill brightness boost stop using a warm-to-neutral color:
```cpp
// After the warmExtY stop (at ~20% from top), add a mid-zone brightness stop
const float normMidFill = dbToNorm (-24.0f);  // -24 dBFS = 60% fill
const float midFillY    = barTop2 + barH2 * (1.0f - normMidFill);
idleGrad.addColour ((midFillY - barTop2) / barH2,
                    MLIMColours::meterWarning.withAlpha (0.35f));  // warm yellow brightness at -24 dB
```

This adds a warm brightness peak at the -24 dBFS level (60% fill point) that creates visible
yellow at the mid-fill zone matching the reference's active fill appearance at ~-14 to -24 dBFS.

The gradient then tapers from this brightness down to `meterSafe.withAlpha(0.85f)` at the
bottom, creating the blue-safe-zone appearance at the base.

Measure Right zone RMSE before and after. Adjust the midFillY dB threshold and alpha to
minimize RMSE. Start with normMidFill = dbToNorm(-24.0f) and alpha 0.35.

Right zone measurement:
```bash
convert /tmp/task-mlim.png -crop 100x400+800+50 +repage /tmp/z-right.png
convert /tmp/task-ref.png  -crop 100x400+800+50 +repage /tmp/r-right.png
compare -metric RMSE /tmp/r-right.png /tmp/z-right.png /dev/null 2>&1
```

**Note**: This task depends on the warm-bleed fix being applied first. Apply that task's
gradient changes before adding the mid-fill brightness stop here.

## Dependencies
Requires: level-meter-idle-gradient-fix-warm-bleed task
