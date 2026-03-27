# Task 417: Level Meter Idle Gradient — Boost Warm Zone Alpha (Right Zone RMSE)

## Description
The level meter idle gradient in `LevelMeter.cpp::drawChannel()` uses alpha=0.10 for
both the danger zone (top, 0 to -0.5 dBFS) and warning zone (-0.5 to -3 dBFS).
This makes the warm top portion nearly invisible at idle/silence.

Pixel analysis of the Right zone (x=720–900) shows:
- Mid 40% average: M-LIM #3E3D49 (R=62, G=61, B=73 — cool blue) vs Reference #473F39 (R=71, G=63, B=57 — warm amber/brown)
- The reference meter shows warm amber/yellow content even at idle, M-LIM shows cold blue-gray

The reference Pro-L 2 meter has visible warm (amber/yellow) segment lighting throughout its
upper zone even without audio playing, simulating the active program state.

Wave-22 baseline Right zone RMSE = 23.50%.

## Produces
None

## Consumes
None

## Relevant Files
Modify: `src/ui/LevelMeter.cpp` — `drawChannel()` idle gradient section (lines ~88–116)
Read:   `src/ui/Colours.h` — available color constants (grMeterLow, grMeterMid, meterWarning, meterSafe)

## Acceptance Criteria
- [ ] Run: `cmake --build build --target MLIM_Standalone_Standalone -j$(nproc)` → Expected: compiles without error
- [ ] Run: Right zone RMSE (crop 180x500+720+0) → Expected: Right RMSE < 23.50% baseline
- [ ] Run: Full RMSE → Expected: Full ≤ 19.11% (no regression)

## Tests
None

## Technical Details
In `LevelMeter.cpp drawChannel()`, idle structural gradient block (lines ~102–116):

**Current (too cool/transparent at warm zones):**
```cpp
idleGrad.addColour(..., MLIMColours::grMeterMid.withAlpha(0.10f));              // orange, near-transparent
idleGrad.addColour(..., MLIMColours::meterWarning.withAlpha(0.10f));            // yellow, near-transparent
idleGrad.addColour(..., MLIMColours::meterSafe.brighter(0.15f).withAlpha(0.45f)); // cool blue
```

**Target: boost warm zone alphas and shift mid-zone warmer:**
```cpp
idleGrad.addColour(..., MLIMColours::grMeterMid.withAlpha(0.28f));              // danger (was 0.10)
idleGrad.addColour(..., MLIMColours::meterWarning.withAlpha(0.25f));            // warning (was 0.10)
idleGrad.addColour(..., MLIMColours::grMeterLow.withAlpha(0.40f));              // warmExtY — warm yellow (was cool blue 0.45)
// Bottom: see task 416 for the separate bottom darkening
```

The key insight: the reference meter mid-40% is R=71, G=63, B=57 (warm/brownish).
M-LIM mid-40% is R=62, G=61, B=73 (cool/bluish). Shifting from blue to amber colors
at mid-range (alpha 0.25–0.35) should bring color temperature closer.

Measure the right zone RMSE before and after. Adjust alpha values incrementally to
minimize RMSE without overshooting into the danger zone being too bright.

Right zone RMSE measurement:
```bash
convert /tmp/task-mlim.png -crop 180x500+720+0 +repage /tmp/cur-right.png
convert /tmp/task-ref.png  -crop 180x500+720+0 +repage /tmp/ref-right.png
compare -metric RMSE /tmp/ref-right.png /tmp/cur-right.png /dev/null 2>&1
```

## Dependencies
None
