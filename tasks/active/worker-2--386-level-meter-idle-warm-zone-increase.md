# Task: Level Meter Idle Warm Zone Transition Alpha Increase

## Description

The level meter idle gradient (in `LevelMeter::drawChannel()`) has a "warm/cool
transition" stop at -12 dBFS using:

```cpp
idleGrad.addColour ((warmExtY - barTop2) / barH2,
                    MLIMColours::meterSafe.brighter (0.15f).withAlpha (0.45f));
```

This stop creates a visible steel-blue color at the -12 dBFS position that transitions
toward the brighter bottom stop. Task 382 found that increasing the bottom stop alpha
from 0.44f to 0.60f improved the right region RMSE. Testing the warm transition stop
from 0.45f → 0.60f is the logical next step.

The reference right panel, even at idle, shows more warm ambient color in the mid-meter
zone. Increasing this transition alpha makes the warm-to-blue transition slightly brighter
and more visible, better approximating the reference's ambient lighting.

**Fix:** In `LevelMeter::drawChannel()`, change warm extension stop alpha:

```cpp
// BEFORE:
MLIMColours::meterSafe.brighter (0.15f).withAlpha (0.45f)
// AFTER:
MLIMColours::meterSafe.brighter (0.15f).withAlpha (0.60f)
```

**Expected improvement:** right region RMSE ~0.1–0.2pp.

## Produces
None

## Consumes
None

## Relevant Files
Modify: `src/ui/LevelMeter.cpp` — `drawChannel()` method, idle gradient definition
Read: `src/ui/Colours.h` — `meterSafe` constant

## Acceptance Criteria
- [ ] Run: build + screenshot + right panel RMSE → Expected: ≤ 22.2% (improvement from 22.36%)
- [ ] Run: full image RMSE → Expected: ≤ 20.74% (no regression from wave 18 baseline)
- [ ] Run: visual check → Expected: level meter shows slightly brighter warm/blue transition at idle

## Tests
None

## Technical Details

In `LevelMeter::drawChannel()`, locate the idle gradient comment block:
```cpp
// Warm/cool transition extended to -12 dB: ramp up to visible
idleGrad.addColour ((warmExtY   - barTop2) / barH2,
                    MLIMColours::meterSafe.brighter (0.15f).withAlpha (0.45f));  // transition
```

Change `0.45f` to `0.60f` on that line only.

Build command:
```bash
export CCACHE_DIR=/build-cache
cmake --build /workspace/M-LIM/build --target MLIM_Standalone -j$(nproc)
```

Measure:
```bash
convert /reference-docs/reference-screenshots/prol2-main-ui.jpg \
    -crop 1712x1073+97+32 +repage -resize 900x500! /tmp/ref.png

pkill -f "Standalone/M-LIM" 2>/dev/null; sleep 1
DISPLAY=:99 /workspace/M-LIM/build/MLIM_artefacts/Release/Standalone/M-LIM &>/dev/null &
sleep 8; DISPLAY=:99 scrot /tmp/raw.png; pkill -f "Standalone/M-LIM"
convert /tmp/raw.png -crop 908x500+509+325 +repage -resize 900x500! /tmp/mlim.png

# Right panel RMSE
convert /tmp/mlim.png -crop 180x500+720+0 +repage /tmp/right-mlim.png
convert /tmp/ref.png  -crop 180x500+720+0 +repage /tmp/right-ref.png
compare -metric RMSE /tmp/right-ref.png /tmp/right-mlim.png /dev/null 2>&1

# Full RMSE
compare -metric RMSE /tmp/ref.png /tmp/mlim.png /dev/null 2>&1
```

If right RMSE worsens vs 22.36%, revert. If it improves, keep.
Fallback: try 0.52f if 0.60f overshoots.

## Dependencies
None
