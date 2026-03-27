# Task: Level meter idle gradient — boost warm zone alpha (Right zone RMSE)

## Description
The level meter idle gradient in `LevelMeter.cpp::drawChannel()` uses alpha=0.10 for
both the danger zone (top, 0 to -0.5 dBFS) and warning zone (-0.5 to -3 dBFS).
This makes the warm top portion nearly invisible at idle/silence.

Pixel analysis of the Right zone (x=720–900) shows:
- Mid 40% average: M-LIM #3E3D49 (cool blue) vs Reference #473F39 (warm amber/brown)
- The reference meter shows warm amber/yellow content even at idle, M-LIM shows cold blue-gray

The reference Pro-L 2 meter has visible warm (amber/yellow) segment lighting throughout
its upper zone even without audio playing, simulating the active program state.

Increase the idle gradient alpha for warm zones and shift the mid-zone transition color
from cool blue to warm amber to close this color temperature gap.

## Produces
None

## Consumes
None

## Relevant Files
Modify: `M-LIM/src/ui/LevelMeter.cpp` — `drawChannel()` function, idle gradient section (lines 88–116)
Read: `M-LIM/src/ui/Colours.h` — available color constants (grMeterLow, grMeterMid, meterWarning, meterSafe)

## Acceptance Criteria
- [ ] Run: build → Expected: compiles without error
- [ ] Run: Right zone RMSE measurement → Expected: Right RMSE < 23.50% baseline
- [ ] Run: Full RMSE → Expected: Full ≤ 19.11% (no regression)

## Tests
None

## Technical Details
In `LevelMeter.cpp drawChannel()`, the idle structural gradient block (lines 102–116):

**Current (too cool/transparent at warm zones):**
```cpp
idleGrad.addColour(..., MLIMColours::grMeterMid.withAlpha(0.10f));     // orange, near-transparent
idleGrad.addColour(..., MLIMColours::meterWarning.withAlpha(0.10f));   // yellow, near-transparent
idleGrad.addColour(..., MLIMColours::meterSafe.brighter(0.15f).withAlpha(0.45f)); // cool blue
```

**Target: boost warm zone alphas and shift mid-zone warmer:**
- Danger zone (0 dBFS): `grMeterMid.withAlpha(0.28f)` (was 0.10)
- Warning zone (-0.5 to -3 dB): `meterWarning.withAlpha(0.25f)` (was 0.10)
- warmExtY transition (-12 dB): use `MLIMColours::grMeterLow.withAlpha(0.40f)` (warm yellow, was cool blue at 0.45)
- Bottom: `meterSafe.darker(0.3f).withAlpha(0.45f)` (was 0.60f, see separate task for bottom darkening)

Measure the right zone RMSE before and after. Adjust alpha values incrementally to
minimize RMSE without overshooting into the danger zone being too bright.

The key insight is the reference meter mid-40% is at R=71, G=63, B=57 (warm/brownish).
M-LIM mid-40% is R=62, G=61, B=73 (cool/bluish). Shifting from blue to amber colors
at mid-range (alpha 0.25-0.35) should bring the color temperature closer.

## Dependencies
None
