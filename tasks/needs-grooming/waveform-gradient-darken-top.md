# Task: Waveform Gradient — Darken Top to Match Reference

## Description

Y-band pixel analysis shows our waveform gradient top is **3x too bright** compared to the reference:
- Our display at y=60 (15% from top): RGB (102, 99, 106) from `displayGradientTop = #686468`
- Reference at same position: RGB (39, 34, 40) — approximately 3x darker

The reference waveform background at the top (near 0 dB, minimal GR zone) is very dark because there
are no audio fills there (the input signal fills the display from bottom upward to the signal level).
Our gradient top #686468 represents the calibrated composite average, but that was overfit and the top
of the display is genuinely darker in the reference.

**Fix**: Darken `displayGradientTop` from `0xff686468` to approximately `0xff252030` (37, 32, 48).

Pixel analysis confirms this would reduce top-zone error from ~65 to ~10 per channel at y=60, and
from ~45 to ~0 per channel at y=130. This affects ~30% of the waveform display height.

**Important**: This task should be combined with the waveform-idle-fill-simulation task, since
darkening the top may slightly increase error in the middle zone (y=150-200) where audio fills in
the reference brighten that area. The fill simulation task offsets this by adding brightness to the
lower portion.

## Produces
None

## Consumes
None

## Relevant Files
Modify: `src/ui/Colours.h` — `displayGradientTop` constant
Skip: all other files — this is a one-line colour constant change

## Acceptance Criteria
- [ ] Run: Wave RMSE measurement → Expected: wave region RMSE below 20% (down from 21.45%)
- [ ] Run: Full RMSE → Expected: full RMSE below 22% (down from 22.25%)
- [ ] Run: Visual inspection → Expected: waveform top area appears dark (near-black) rather than medium warm-gray

## Tests
None

## Technical Details

**In `src/ui/Colours.h`, change:**
```cpp
// CURRENT (too bright at top, causes high error in y=60-160 zone):
const juce::Colour displayGradientTop   { 0xff686468 };  // neutral/warm gray

// CHANGE TO (dark blue-purple, matches reference at top of waveform):
const juce::Colour displayGradientTop   { 0xff252030 };  // dark blue-purple, ~14% brightness
```

**Fine-tuning range**: Values between `0xff202030` and `0xff303540` are reasonable. Run RMSE after
each value to find the sweet spot. Lighter values (0xff303540 etc.) are safer if visual inspection
looks too dark.

**Measurement methodology:**
```bash
# Build and screenshot (per standard methodology)
cd /workspace/M-LIM && export CCACHE_DIR=/build-cache
cmake --build build --target MLIM_Standalone -j$(nproc)
DISPLAY=:99 /workspace/M-LIM/build/MLIM_artefacts/Release/Standalone/M-LIM &
sleep 8
DISPLAY=:99 scrot /tmp/mlim-task.png
pkill -f "Standalone/M-LIM"

# Crop
convert /tmp/mlim-task.png -crop 908x500+509+325 +repage -resize 900x500! /tmp/mlim-c.png
convert /reference-docs/reference-screenshots/prol2-main-ui.jpg -crop 1712x1073+97+32 +repage -resize 900x500! /tmp/ref-c.png

# Wave RMSE
convert /tmp/mlim-c.png -crop 640x500+0+0 +repage /tmp/mlim-wave.png
convert /tmp/ref-c.png -crop 640x500+0+0 +repage /tmp/ref-wave.png
compare -metric RMSE /tmp/mlim-wave.png /tmp/ref-wave.png /dev/null 2>&1

# Full RMSE
compare -metric RMSE /tmp/mlim-c.png /tmp/ref-c.png /dev/null 2>&1
```

**NOTE**: If wave RMSE worsens (possible if the fill simulation isn't applied simultaneously),
try darkening less aggressively: `0xff3A3540` → `0xff3A3545`. The goal is to avoid making the
already-relatively-dark bottom zone even harder to match.

## Dependencies
Can run in parallel with waveform-idle-fill-simulation. ProjectManager should assign both to the
same worker or run them sequentially so their combined effect can be measured.
