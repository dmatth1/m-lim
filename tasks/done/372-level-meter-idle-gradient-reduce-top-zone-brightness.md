# Task 372: Level Meter — Reduce Top Zone Brightness of Idle Gradient

## Description

The output level meter idle gradient (task 365) applies warm colors at 0.44f alpha across the full
bar height, including the top zone (near 0 dBFS). This causes the top of each meter bar to show
bright reddish content (composite: R=132, G=47, B=49), while the reference shows a dark empty
track at the top (the reference was captured at ~-14 LUFS, so most of the meter is active in the
lower 80% but the top 20% is dark/empty).

**Pixel analysis (right panel top 200px):**
- Reference avg: R=47.6, G=39.0, B=41.3 (dark, warm-ish)
- M-LIM avg:    R=56.6, G=54.7, B=62.8 (brighter, more blue)
- Right panel full RMSE: 23.94%

The mismatch at the top comes from the idle gradient applying `meterDanger.withAlpha(0.44f)` (red)
from the very top of the bar, compositing over `barTrackBackground=#231417` to give a visible
reddish color at y=0. The reference has just the dark track color there.

**Fix:** Taper the idle gradient alpha so that the top ~15% of the bar (above -3 dBFS danger zone)
uses lower alpha (0.10f–0.15f) instead of 0.44f, preserving the warm lower zone that improved
the right panel RMSE while reducing the artificial brightness at the top.

Specifically, add a near-transparent stop at y=0 for the meterDanger color, and another stop at
the danger zone boundary with full 0.44f alpha. This makes the very top of the meter darker (matching
reference's empty track) while maintaining the warm orange/yellow zone below.

## Produces
None

## Consumes
None

## Relevant Files
Modify: `src/ui/LevelMeter.cpp` — `drawChannel()` method, the idle structural gradient block (~line 89)
Read: `src/ui/Colours.h` — color constants (meterDanger, grMeterMid, meterWarning, meterSafe)

## Acceptance Criteria
- [x] Run: right panel RMSE → Result: 23.57% ✓ (≤23.94%, improved -0.37pp)
- [x] Run: full RMSE → Result: 21.22% ✓ (≤21.30%, improved -0.08pp)
- [x] Run: left meter RMSE → Not measured (level meter in right panel only, not affected)
- [x] Run: Visual inspection → top zone alpha 0.44→0.10 through warn boundary, dark/empty at top

## Tests
None

## Technical Details

**In `src/ui/LevelMeter.cpp`, in `drawChannel()`, modify the idle gradient:**

```cpp
// CURRENT: red at full 0.44f alpha from the very top
juce::ColourGradient idleGrad (
    MLIMColours::meterDanger.withAlpha (0.44f),              0.0f, barTop2,  // ← too bright at top
    MLIMColours::meterSafe.darker (0.3f).withAlpha (0.44f),  0.0f, barTop2 + barH2,
    false);
idleGrad.addColour ((dangerBot2 - barTop2) / barH2,
                    MLIMColours::grMeterMid.withAlpha (0.44f));
idleGrad.addColour ((warnBot2   - barTop2) / barH2,
                    MLIMColours::meterWarning.withAlpha (0.44f));
idleGrad.addColour ((warmExtY   - barTop2) / barH2,
                    MLIMColours::meterSafe.brighter (0.15f).withAlpha (0.30f));

// REPLACE WITH: taper alpha from 0.10 at very top to 0.44 at danger boundary
juce::ColourGradient idleGrad (
    MLIMColours::meterDanger.withAlpha (0.10f),              0.0f, barTop2,  // near-transparent at top
    MLIMColours::meterSafe.darker (0.3f).withAlpha (0.44f),  0.0f, barTop2 + barH2,
    false);
// Full alpha begins at the danger zone boundary (not at bar top)
idleGrad.addColour ((dangerBot2 - barTop2) / barH2,
                    MLIMColours::grMeterMid.withAlpha (0.44f));
idleGrad.addColour ((warnBot2   - barTop2) / barH2,
                    MLIMColours::meterWarning.withAlpha (0.44f));
idleGrad.addColour ((warmExtY   - barTop2) / barH2,
                    MLIMColours::meterSafe.brighter (0.15f).withAlpha (0.30f));
```

**Rationale:** The alpha change at y=0 only affects the very top of the bar (above -3 dBFS). The
warm zone (orange/yellow, -3 dBFS to -12 dBFS) and blue zone (-12 dBFS to bottom) keep 0.44f alpha.
This preserves the warm-program-material appearance in the active zone while making the top match
the reference's dark empty track.

**Measurement methodology:**
```bash
# Build standalone
cd /workspace/M-LIM && export CCACHE_DIR=/build-cache
cmake --build build --target MLIM_Standalone -j$(nproc)

# Launch and screenshot
pkill -f "Standalone/M-LIM" 2>/dev/null; sleep 1
DISPLAY=:99 /workspace/M-LIM/build/MLIM_artefacts/Release/Standalone/M-LIM &
sleep 8
DISPLAY=:99 scrot /tmp/task-raw.png
pkill -f "Standalone/M-LIM"

# Correct crops
convert /reference-docs/reference-screenshots/prol2-main-ui.jpg \
    -crop 1712x1073+97+32 +repage -resize 900x500! /tmp/ref.png
convert /tmp/task-raw.png -crop 908x500+509+325 +repage -resize 900x500! /tmp/mlim.png

# Full RMSE
compare -metric RMSE /tmp/ref.png /tmp/mlim.png /dev/null 2>&1

# Right panel RMSE (180px, x=720-900)
convert /tmp/mlim.png -crop 180x500+720+0 +repage /tmp/mlim-right.png
convert /tmp/ref.png  -crop 180x500+720+0 +repage /tmp/ref-right.png
compare -metric RMSE /tmp/mlim-right.png /tmp/ref-right.png /dev/null 2>&1
```

**Wave 15 baseline:** Full=21.30%, Right=23.94%

## Dependencies
None
