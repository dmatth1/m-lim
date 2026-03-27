# Task 365: Level Meter Idle Ambient Fill — Boost Idle Gradient Brightness and Warmth

## Description

The output level meter bars are too dark at idle. Pixel analysis shows:
- **Our level meter (idle):** mean (46, 45, 55) ≈ #2E2D37 (18% brightness)
- **Reference (active audio):** mean (119, 108, 94) ≈ #776C5E (44% brightness, warm orange-brown)

The idle gradient alpha is currently 0.15f — barely visible. The reference was captured with loud audio, so
its meter shows mostly the warm (red/orange/yellow) upper zone. We need to:

1. **Increase idle gradient alpha from 0.15f → 0.44f** across all stops
2. **Add an orange intermediate stop** between danger (red) and warning (yellow) — this makes the idle
   warm tone closer to the reference's warm orange-brown character
3. **Extend the idle warm zone further down the bar** — instead of using `kWarnDB` (-3 dB) as the
   yellow→blue transition, use a lower threshold (-18 dB) so 70% of the bar height shows yellow/orange
   rather than blue. This simulates the appearance of typical loud-program-material metering.

The change should dramatically reduce right panel RMSE (currently 29.26%).

## Produces
None

## Consumes
None

## Relevant Files
Modify: `src/ui/LevelMeter.cpp` — `drawChannel()` method, the idle structural gradient block
Read: `src/ui/Colours.h` — `grMeterMid` (orange #FF8C00), `meterDanger`, `meterWarning`, `meterSafe`
Skip: `src/ui/LevelMeter.h` — no changes needed

## Acceptance Criteria
- [ ] Run: `compare -metric RMSE /tmp/mlim-crop.png /tmp/ref-crop.png /dev/null 2>&1` after building and screenshotting → Expected: right panel sub-region RMSE below 24% (down from 29.26%)
- [ ] Run: Visual inspection of screenshot → Expected: level meter bars visibly lit up with warm gradient at idle, not nearly black
- [ ] Run: Full RMSE → Expected: full RMSE below 21.5% (down from 22.25%)

## Tests
None

## Technical Details

**In `src/ui/LevelMeter.cpp`, find the idle structural gradient block (~line 89):**

```cpp
// CURRENT (alpha = 0.15, only 4 stops, narrow warm zone):
juce::ColourGradient idleGrad (
    MLIMColours::meterDanger.withAlpha (0.15f),             0.0f, barTop2,
    MLIMColours::meterSafe.darker (0.3f).withAlpha (0.15f), 0.0f, barTop2 + barH2,
    false);
idleGrad.addColour ((dangerBot2 - barTop2) / barH2,
                    MLIMColours::meterWarning.withAlpha (0.15f));
idleGrad.addColour ((warnBot2   - barTop2) / barH2,
                    MLIMColours::meterSafe.brighter (0.15f).withAlpha (0.15f));

// REPLACE WITH (alpha = 0.44, orange stop added, warm zone extended to -18 dB):
// Extended warm zone: use a lower threshold than kWarnDB for the idle gradient
// so the orange/yellow zone covers ~70% of the bar height (simulating loud-program look)
const float normWarmExt  = dbToNorm (-18.0f);  // extended warm/cool boundary for idle
const float warmExtY     = barTop2 + barH2 * (1.0f - normWarmExt);  // ~70% from top

juce::ColourGradient idleGrad (
    MLIMColours::meterDanger.withAlpha (0.44f),              0.0f, barTop2,
    MLIMColours::meterSafe.darker (0.3f).withAlpha (0.44f),  0.0f, barTop2 + barH2,
    false);
// Add orange stop just below the danger/warning boundary
idleGrad.addColour ((dangerBot2 - barTop2) / barH2,
                    MLIMColours::grMeterMid.withAlpha (0.44f));     // orange #FF8C00
// Yellow at kWarnDB boundary
idleGrad.addColour ((warnBot2   - barTop2) / barH2,
                    MLIMColours::meterWarning.withAlpha (0.44f));
// Warm/cool transition extended to -18 dB (instead of kWarnDB)
idleGrad.addColour ((warmExtY   - barTop2) / barH2,
                    MLIMColours::meterSafe.brighter (0.15f).withAlpha (0.30f));  // transition
```

**Measurement methodology (use correct RMSE crop):**
```bash
# Build
cd /workspace/M-LIM && export CCACHE_DIR=/build-cache && cmake --build build --target MLIM_Standalone -j$(nproc)

# Screenshot
DISPLAY=:99 /workspace/M-LIM/build/MLIM_artefacts/Release/Standalone/M-LIM &
sleep 8
DISPLAY=:99 scrot /tmp/mlim-wave15-task.png
pkill -f "Standalone/M-LIM"

# Crop (per methodology)
convert /tmp/mlim-wave15-task.png -crop 908x500+509+325 +repage -resize 900x500! /tmp/mlim-crop-task.png
convert /reference-docs/reference-screenshots/prol2-main-ui.jpg -crop 1712x1073+97+32 +repage -resize 900x500! /tmp/ref-crop.png

# Full RMSE
compare -metric RMSE /tmp/mlim-crop-task.png /tmp/ref-crop.png /dev/null 2>&1

# Right panel RMSE (x=720-900)
convert /tmp/mlim-crop-task.png -crop 180x500+720+0 +repage /tmp/mlim-right.png
convert /tmp/ref-crop.png -crop 180x500+720+0 +repage /tmp/ref-right.png
compare -metric RMSE /tmp/mlim-right.png /tmp/ref-right.png /dev/null 2>&1
```

**If RMSE worsens or bars look artificially "always-on":**
- Try alpha 0.38f instead of 0.44f
- Reduce the extended warm zone to -12 dB instead of -18 dB
- Fall back to just the alpha increase (0.44f) without warm-zone extension

**Save results to:** `screenshots/task-NNN-rmse-results.txt`

## Dependencies
None
