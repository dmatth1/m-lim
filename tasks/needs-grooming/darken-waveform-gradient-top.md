# Task: Darken Waveform Top Gradient to Match Reference

## Description
The waveform display top background is too light. Pixel analysis shows:
- M-LIM waveform top (y=50 of comparison crop): `#3C3A49` (R=60, G=58, B=73)
- Reference at same position: `#262127` (R=38, G=33, B=39)

The `displayGradientTop` constant (`0xff3A3540`) is ~20-25 pixel units too bright
in every channel. Darkening it to approximately `0xff242027` would bring M-LIM much
closer to the reference's very dark purple-gray top. This constant is decoupled from
`loudnessHistogramTop` (separate constant since task-371), so this change only affects
the waveform display background.

**Expected RMSE gain:** Wave region −0.5 to −0.8pp, Full −0.2pp.

## Produces
None

## Consumes
None

## Relevant Files
Modify: `src/ui/Colours.h` — change `displayGradientTop` from `0xff3A3540` to `0xff242027`
Read: `src/ui/WaveformDisplay.cpp` — confirms gradient usage in `drawBackground()`
Skip: `src/ui/LoudnessPanel.cpp` — uses `loudnessHistogramTop` (separate constant, no change needed)

## Acceptance Criteria
- [ ] Run: build then capture screenshot → crop → measure Wave RMSE
  → Expected: Wave RMSE ≤ 19.0% (down from 19.44%)
- [ ] Run: Full RMSE measurement → Expected: Full ≤ 21.1% (down from 21.22%)
- [ ] Run: visual check of waveform top → Expected: darker top background matching reference #262127

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

**Why this value:** Reference waveform background at top is `#262127` (R=38,G=33,B=39).
The new constant `0xff242027` = R=36,G=32,B=39 matches within ±2 counts. The gradient
is applied top→bottom in the waveform display only (not the loudness panel).

**Measurement command:**
```bash
convert /reference-docs/reference-screenshots/prol2-main-ui.jpg \
    -crop 1712x1073+97+32 +repage -resize 900x500! /tmp/ref.png

DISPLAY=:99 /workspace/M-LIM/build/MLIM_artefacts/Release/Standalone/M-LIM &
APP_PID=$!; sleep 6
DISPLAY=:99 scrot /tmp/shot.png
kill $APP_PID

convert /tmp/shot.png -crop 908x500+509+325 +repage -resize 900x500! /tmp/mlim.png

# Full RMSE
compare -metric RMSE /tmp/ref.png /tmp/mlim.png /dev/null 2>&1

# Wave RMSE (x=0-640)
convert /tmp/mlim.png -crop 640x500+0+0 +repage /tmp/wave.png
convert /tmp/ref.png  -crop 640x500+0+0 +repage /tmp/ref_wave.png
compare -metric RMSE /tmp/ref_wave.png /tmp/wave.png /dev/null 2>&1
```

## Dependencies
None
