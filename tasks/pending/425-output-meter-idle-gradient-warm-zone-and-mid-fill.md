# Task 425: Output Meter Idle Gradient — Warm Zone Alpha and Mid-Fill Brightness

## Description
The output meter idle gradient (Right zone, x=800–900, crop 100x400+800+50) shows
26.74% RMSE. Two issues combined:

**Issue 1** — Warm zone too dark overall:
- Reference avg: `#70665A` (R=112, G=102, B=90)
- M-LIM current: `#564B39` (R=86, G=75, B=57)
- Gap: ~26–33 units darker

**Issue 2** — Missing mid-fill brightness peak:
At y=200 (50% height ≈ -30 dBFS), the reference shows very bright warm fill
(R=149, G=125, B=67) simulating active program at -12 dBFS. M-LIM shows only
(69,62,48) — much dimmer.

**Fix 1** — Increase idle gradient warm zone alphas in `LevelMeter.cpp`:
```cpp
// grMeterMid stop:    0.28f → 0.48f
// meterWarning stop:  0.25f → 0.40f
// grMeterLow stop:    0.20f → 0.35f
// bottom meterSafe:   alpha(0.80f) → alpha(0.95f)
```

**Fix 2** — Add a mid-fill brightness boost at the -24 dBFS level (~60% fill):
```cpp
const float normMidFill = dbToNorm (-24.0f);
const float midFillY    = barTop2 + barH2 * (1.0f - normMidFill);
idleGrad.addColour ((midFillY - barTop2) / barH2,
                    MLIMColours::meterWarning.withAlpha (0.35f));
```

Apply both fixes together. Measure Right zone RMSE and Full RMSE before committing.

**Note**: Task 424 (warm bleed fix) adjusts the SAME `LevelMeter.cpp` gradient block for
the input meter. This task operates on the OUTPUT meter section (right channel bar rendering,
which may be a separate drawChannel call or a separate code path). Verify which section of
`drawChannel()` handles the output/right bar and make targeted changes there only to avoid
undoing the warm-bleed fix for the input meter.

## Produces
None

## Consumes
None

## Relevant Files
Modify: `src/ui/LevelMeter.cpp` — `drawChannel()` idle gradient block (lines ~88–117)
Read: `src/ui/Colours.h` — `meterSafe`, `meterWarning`, `grMeterLow`, `grMeterMid`, `barTrackBackground`

## Acceptance Criteria
- [ ] Run: `cmake --build build --target MLIM_Standalone_Standalone -j$(nproc) 2>&1 | tail -3` → Expected: build succeeds, exit 0
- [ ] Run: Right zone RMSE (crop 100x400+800+50) → Expected: ≤ 24.00% (improvement from 26.74%)
- [ ] Run: Full RMSE → Expected: ≤ 19.50%
- [ ] Run: Left zone RMSE (crop 30x378+0+30) → Expected: ≤ 25.00% (no regression from task 424)

## Tests
None

## Technical Details
Build Standalone: `cmake --build build --target MLIM_Standalone_Standalone -j$(nproc)`.

Right zone RMSE:
```bash
convert /tmp/task-mlim.png -crop 100x400+800+50 +repage /tmp/z-right.png
convert /tmp/task-ref.png  -crop 100x400+800+50 +repage /tmp/r-right.png
compare -metric RMSE /tmp/r-right.png /tmp/z-right.png /dev/null 2>&1
```

## Dependencies
Requires task 424
