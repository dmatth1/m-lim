# Task: Output Meter Idle Gradient Rework — Darken Top, Brighten Bottom

## Description

Task 365 boosted the output meter idle gradient alpha to 0.44 with warm orange/yellow/red at the top
(danger/warning zones). Pixel analysis shows this creates HIGHER diff at the TOP of the meter where
the reference is DARK, while still leaving the BOTTOM too dark where the reference shows light blue
active meter fill.

**Pixel analysis (x=830, output meter column):**

| y | Reference | M-LIM current | Diff | Issue |
|---|-----------|---------------|------|-------|
| y=60–80 | #352329–#324027 (dark) | #83581C–#7E6531 (warm golden) | ~50 | Warm gradient hurts |
| y=100–170 | #311F25–#1D161A (near-black) | #53473D–#3C3643 (warm dark) | ~28–36 | Warm gradient hurts |
| y=300–420 | #3E3A3D–#909BC2 (dark to light blue) | #393644–#484554 (purple-dark) | 5–89 | Bottom too dark |
| y=340–390 | #CFD4E7–#D9E3FF (light blue, ~217,227,255) | #383443 (dark purple) | ~174 | Major gap at bottom |

The top of the meter (y=40–130, corresponding to -0.5 to -3 dB range) should be VERY DARK because
the reference shows no active audio fills there (the audio isn't that loud at the very peak).
The bottom of the meter (y=300–420, corresponding to -20 to -50 dB) should be MUCH BRIGHTER because
the reference shows active meter fill in the safe zone (light blue-gray #707690 to #D9E3FF).

**Root cause:** Task 365 used warm colors (red/orange/yellow) at the top at 0.44 alpha, which creates
amber warmth (~#7E6531) where reference is near-black. Meanwhile the bottom's meterSafe at 0.44 alpha
over barTrackBackground gives only ~#383443, far from the reference's ~#D9E3FF.

## Produces
None

## Consumes
None

## Relevant Files
Modify: `src/ui/LevelMeter.cpp` — `drawChannel()` idle structural gradient block (lines 86–118)
Read: `src/ui/Colours.h` — `meterSafe` (#6879A0), `barTrackBackground` (#231417), `meterDanger`, etc.

## Acceptance Criteria
- [ ] Run: Build + screenshot → full RMSE → Expected: full RMSE ≤ 21.3% (no regression from current)
- [ ] Run: Right panel crop (180×500 at x=720) → Expected: below 23.94% (current)
- [ ] Run: Visual check — top of output meter bars should be dark (not amber/golden); bottom should show muted blue-gray

## Tests
None

## Technical Details

**In `src/ui/LevelMeter.cpp` `drawChannel()`, find the idle gradient block (~line 86–118):**

Current gradient has red/orange/yellow at top at 0.44 alpha. Replace with:
- **Top zone (0% to 5% from top, 0 dB to -3 dB):** very low alpha (0.08) red → orange → yellow. This preserves a subtle color hint without the amber warmth.
- **Warm zone (5% to 25% from top, -3 dB to -12 dB):** alpha 0.12 yellow → transitional
- **Safe zone (25% to 100% from top, -12 dB to min):** alpha ramping from 0.45 at 25% to 0.70 at 100%. Use `meterSafe.brighter(0.25)` as color. This simulates a meter filled to -14 dBFS.

```cpp
// REPLACE the idle structural gradient block with:
{
    const float barTop2 = bar.getY();
    const float barH2   = bar.getHeight();

    const float normWarn2    = dbToNorm (kWarnDB);    // ~-3 dB = 0.95
    const float normDanger2  = dbToNorm (kDangerDB);  // ~-0.5 dB = 0.992
    const float dangerBot2   = barTop2 + barH2 * (1.0f - normDanger2);  // ~0.8% from top
    const float warnBot2     = barTop2 + barH2 * (1.0f - normWarn2);    // ~5% from top

    // Bright safe zone: simulate level meter appearing filled to -12 dB
    // This makes the bottom 75% of the bar show a soft muted-blue "filled" appearance.
    const float normSafeStart = dbToNorm (-12.0f);  // 0.80
    const float safeStartY    = barTop2 + barH2 * (1.0f - normSafeStart);  // 20% from top

    juce::ColourGradient idleGrad (
        MLIMColours::meterDanger.withAlpha (0.08f),                          0.0f, barTop2,
        MLIMColours::meterSafe.brighter (0.25f).withAlpha (0.68f),           0.0f, barTop2 + barH2,
        false);
    // Danger zone: subtle red → subtle orange at very low alpha
    idleGrad.addColour ((dangerBot2 - barTop2) / barH2,
                        MLIMColours::grMeterMid.withAlpha (0.08f));
    // Warning zone: subtle yellow
    idleGrad.addColour ((warnBot2 - barTop2) / barH2,
                        MLIMColours::meterWarning.withAlpha (0.10f));
    // Transition from warm to cool
    idleGrad.addColour ((safeStartY - barTop2) / barH2,
                        MLIMColours::meterSafe.withAlpha (0.40f));

    g.setGradientFill (idleGrad);
    g.fillRect (bar);
}
```

**Expected composite at key positions** (over barTrackBackground #231417 = 35,20,23):
- y=80 (top ~5%): 0.08 * orange (#FF8C00) = (255,140,0) → composite ~(36,23,23) ≈ #242017. Near-black. ✓
- y=200 (mid ~38%): 0.40 * meterSafe = 0.40*(104,121,160) → composite ~(56,61,87) ≈ #383D57. Better than current. ✓
- y=350 (lower ~72%): 0.68 * meterSafe.brighter(0.25) ≈ 0.68*(130,151,200) → composite ~(100,115,148) ≈ #647394.
  Reference is (217,227,255). Still gap but MUCH better than current (56,52,67).

**If RMSE worsens significantly:** Fall back to alpha 0.60 for the bottom and 0.06 for the top.
Alternatively, test at 0.55 and 0.06 before committing.

**Measurement:** Same methodology as task 365. Save results to `screenshots/task-NNN-rmse-results.txt`.

## Dependencies
None
