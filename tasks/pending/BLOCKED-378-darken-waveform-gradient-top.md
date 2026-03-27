# Task 378: Darken Waveform Top Gradient to Match Reference

## Description

The waveform display top background is too light. Pixel analysis shows:
- M-LIM waveform top (y=50 of comparison crop): `#3C3A49` (R=60, G=58, B=73)
- Reference at same position: `#262127` (R=38, G=33, B=39)

The `displayGradientTop` constant (`0xff3A3540`) is ~20-25 pixel units too bright per
channel. Darkening to approximately `0xff242027` brings M-LIM within ±2 counts of the
reference's dark purple-gray top.

This constant is decoupled from `loudnessHistogramTop` (separate constant since task 371),
so this change only affects the waveform display background — not the loudness panel.

**Expected RMSE gain:** Wave region −0.5 to −0.8pp, Full −0.2pp.

**Note:** Task 381 also modifies `Colours.h` (different constants: knobFaceHighlight/Shadow).
These can run in parallel; each worker edits only their specified constants.

## Produces
None

## Consumes
None

## Relevant Files
Modify: `src/ui/Colours.h` — change `displayGradientTop` from `0xff3A3540` to `0xff242027`
Read: `src/ui/WaveformDisplay.cpp` — confirms gradient usage in `drawBackground()`
Skip: `src/ui/LoudnessPanel.cpp` — uses `loudnessHistogramTop` (separate constant)

## Acceptance Criteria
- [ ] Run: build + screenshot + wave RMSE → Expected: Wave RMSE ≤ 19.0% (from wave 17 baseline)
- [ ] Run: full image RMSE → Expected: ≤ wave-17 full baseline (no regression)
- [ ] Run: visual check → Expected: waveform top is notably darker, matching reference dark purple-gray

## Tests
None

## Technical Details

In `src/ui/Colours.h`, change exactly one line:
```cpp
// Before:
const juce::Colour displayGradientTop   { 0xff3A3540 };

// After:
const juce::Colour displayGradientTop   { 0xff242027 };
```

**Why:** Reference waveform top = `#262127` (R=38,G=33,B=39). New constant `0xff242027`
= R=36,G=32,B=39, within ±2 counts.

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

# Wave RMSE
convert /tmp/mlim.png -crop 640x500+0+0 +repage /tmp/wave.png
convert /tmp/ref.png  -crop 640x500+0+0 +repage /tmp/ref_wave.png
compare -metric RMSE /tmp/ref_wave.png /tmp/wave.png /dev/null 2>&1

# Full RMSE
compare -metric RMSE /tmp/ref.png /tmp/mlim.png /dev/null 2>&1
```

## Dependencies
Requires task 377

## Blocker

Empirical RMSE testing shows that darkening `displayGradientTop` causes regression, not improvement:

- Baseline (task-377): Full=20.81%, Wave=19.04%
- With `0xff242027` (proposed): Full=21.04%, Wave=19.38% — regression +0.23pp/+0.34pp
- With `0xff282030` (intermediate): Full=20.97%, Wave=19.28% — regression +0.16pp/+0.24pp

The constant was fine-tuned in task-368 specifically for wave RMSE. Any darkening from `0xff3A3540` causes measurable regression in both full and wave metrics. The pixel analysis assumed that reference's dark top (#262127) is pure background, but it appears to be a composite including waveform fill that M-LIM does not render in idle state.

Acceptance criteria cannot be met with any darkening. The current value `0xff3A3540` remains optimal.
