# Task 424: Level Meter Idle Gradient — Remove Warm-Amber Bleed, Use Cool Blue

## Description
The level meter idle gradient in `LevelMeter.cpp::drawChannel()` was boosted by tasks 415–417
to add warm orange/yellow colours at idle. This caused a Left zone RMSE regression:
**30.25% vs 23.50% baseline** (task-354).

Root cause: the `warmExtY` stop uses `grMeterLow.withAlpha(0.20f)` (warm yellow #E8C840),
which bleeds through the lower 80% of the bar via JUCE gradient interpolation. At y=25% from
top the result is warm amber ~(82,63,33), but the reference shows cool blue-purple (105,101,141).

Pixel evidence (input meter x=5, idle state):
```
y=200: M-LIM (92,74,42) warm-amber  | Ref (105,101,141) — ref COOL blue-purple
y=300: M-LIM (93,85,67) warm-amber  | Ref (137,143,172) — ref bright cool blue-purple
```

**Fix**: Replace warm-amber bleed with cool blue gradient:
```cpp
juce::ColourGradient idleGrad (
    MLIMColours::meterDanger.withAlpha (0.08f),           0.0f, barTop2,
    MLIMColours::meterSafe.withAlpha (0.85f),             0.0f, barTop2 + barH2,
    false);
idleGrad.addColour ((dangerBot2 - barTop2) / barH2,
                    MLIMColours::grMeterMid.withAlpha (0.10f));     // reduced warm
idleGrad.addColour ((warnBot2   - barTop2) / barH2,
                    MLIMColours::meterWarning.withAlpha (0.08f));   // reduced warm
idleGrad.addColour ((warmExtY   - barTop2) / barH2,
                    MLIMColours::meterSafe.withAlpha (0.10f));      // cool blue replaces warm bleed
```

## Produces
None

## Consumes
None

## Relevant Files
Modify: `src/ui/LevelMeter.cpp` — `drawChannel()` idle gradient block (lines ~88–117)
Read: `src/ui/Colours.h` — `meterSafe`, `barTrackBackground`, `grMeterLow`, `grMeterMid`

## Acceptance Criteria
- [ ] Run: `cmake --build build --target MLIM_Standalone_Standalone -j$(nproc) 2>&1 | tail -3` → Expected: build succeeds, exit 0
- [ ] Run: Left zone RMSE (crop 30x378+0+30) → Expected: ≤ 25.00% (improvement from 30.25%)
- [ ] Run: Full RMSE → Expected: ≤ 19.50%
- [ ] Run: Right zone RMSE (crop 100x400+800+50) → Expected: ≤ 26.74% (no regression)

## Tests
None

## Technical Details
In `LevelMeter.cpp drawChannel()` (~lines 88–117), apply the warm bleed fix shown above.

RMSE measurement commands:
```bash
convert /tmp/task-mlim.png -crop 30x378+0+30  +repage /tmp/z-left.png
convert /tmp/task-ref.png  -crop 30x378+0+30  +repage /tmp/r-left.png
compare -metric RMSE /tmp/r-left.png /tmp/z-left.png /dev/null 2>&1
```

Build Standalone only: `cmake --build build --target MLIM_Standalone_Standalone -j$(nproc)`.

## Dependencies
None
