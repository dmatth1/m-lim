# Task 382: Reduce Level Meter Idle Gradient Bottom Alpha

## Description

The level meter idle structural gradient has its BOTTOM stop at
`meterSafe.darker(0.3f).withAlpha(0.44f)`, producing a visible steel-blue/teal glow at
the bottom of the idle meter bars. Task 372 already reduced the TOP zone alpha from 0.44
to 0.10, but the BOTTOM stop was not changed.

In the reference, idle meters appear near-dark with minimal ambient color. The current
0.44 alpha bottom stop contributes a noticeable blue-teal ambient fill to the lower
portion of the meter bars.

With the top zone at 0.10 alpha (task 372), the bottom at 0.44 creates a gradient ramping
from near-transparent at top to semi-opaque blue at bottom. Making both ends consistently
near-transparent (~0.08) produces a near-dark idle state matching the reference.

**Fix:** In `LevelMeter.cpp::drawChannel()`, change bottom idle gradient stop alpha:
- Bottom stop: `0.44f` → `0.08f`
- Warm extension mid-stop: `0.30f` → `0.08f`

**Expected RMSE gain:** Right panel −0.3pp.

## Produces
None

## Consumes
None

## Relevant Files
Modify: `src/ui/LevelMeter.cpp` — `drawChannel()` method, idle gradient stop alphas
Read: `src/ui/Colours.h` — `meterSafe`, `barTrackBackground` constants

## Acceptance Criteria
- [ ] Run: build + screenshot + right panel RMSE → Expected: < wave-17 right baseline
- [ ] Run: full image RMSE → Expected: ≤ wave-17 full baseline (no regression)
- [ ] Run: visual check → Expected: meter bars appear dark at idle, no visible blue glow at bottom

## Tests
None

## Technical Details

In `LevelMeter::drawChannel()`, locate the idle gradient definition:
```cpp
juce::ColourGradient idleGrad (
    MLIMColours::meterDanger.withAlpha (0.10f),              0.0f, barTop2,
    MLIMColours::meterSafe.darker (0.3f).withAlpha (0.44f),  0.0f, barTop2 + barH2,
    false);
```

Change the bottom stop alpha from `0.44f` to `0.08f`:
```cpp
juce::ColourGradient idleGrad (
    MLIMColours::meterDanger.withAlpha (0.10f),              0.0f, barTop2,
    MLIMColours::meterSafe.darker (0.3f).withAlpha (0.08f),  0.0f, barTop2 + barH2,
    false);
```

Also reduce the warm extension stop alpha from `0.30f` to `0.08f`:
```cpp
idleGrad.addColour ((warmExtY - barTop2) / barH2,
    MLIMColours::meterSafe.brighter (0.15f).withAlpha (0.08f));  // was 0.30f
```

Build and measure:
```bash
export CCACHE_DIR=/build-cache
cmake --build /workspace/M-LIM/build --target MLIM_Standalone_Standalone -j$(nproc)

convert /reference-docs/reference-screenshots/prol2-main-ui.jpg \
    -crop 1712x1073+97+32 +repage -resize 900x500! /tmp/ref.png

pkill -f "Standalone/M-LIM" 2>/dev/null; sleep 1
DISPLAY=:99 /workspace/M-LIM/build/MLIM_artefacts/Release/Standalone/M-LIM &>/dev/null &
sleep 8
DISPLAY=:99 scrot /tmp/raw.png
pkill -f "Standalone/M-LIM"

convert /tmp/raw.png -crop 908x500+509+325 +repage -resize 900x500! /tmp/mlim.png

convert /tmp/mlim.png -crop 180x500+720+0 +repage /tmp/right.png
convert /tmp/ref.png  -crop 180x500+720+0 +repage /tmp/ref_right.png
compare -metric RMSE /tmp/ref_right.png /tmp/right.png /dev/null 2>&1
compare -metric RMSE /tmp/ref.png /tmp/mlim.png /dev/null 2>&1
```

## Dependencies
Requires task 377
