# Task 380: Reduce Waveform Grid Line Alpha

## Description

The horizontal dB grid lines in the waveform display are drawn at `waveformGridLine.withAlpha(0.6f)`,
where `waveformGridLine = 0xff9AA0B4` (light blue-gray). This creates prominent horizontal
stripes across the waveform display — especially visible in the idle (no-audio) state.

In the reference (FabFilter Pro-L 2), the grid lines are obscured by dense waveform content
and appear much more subtle (~15-20% apparent opacity). The M-LIM idle state has no waveform
fill at that opacity, making the grid lines appear overly prominent.

Reducing the alpha from `0.6f` to `0.35f` keeps lines readable but substantially less harsh.

**Expected RMSE gain:** Wave region −0.2pp.

**Note:** This modifies `WaveformDisplay.cpp`. Must run AFTER task 379 (same file).

## Produces
None

## Consumes
None

## Relevant Files
Modify: `src/ui/WaveformDisplay.cpp` — `drawBackground()` method, grid line alpha
Read: `src/ui/Colours.h` — `waveformGridLine = 0xff9AA0B4`

## Acceptance Criteria
- [ ] Run: build + screenshot + wave RMSE → Expected: Wave RMSE decreases from task 379 state
- [ ] Run: full image RMSE → Expected: ≤ wave-17 full baseline (no regression)
- [ ] Run: visual check → Expected: horizontal dB grid lines are visible but subtle, not harsh bright stripes

## Tests
None

## Technical Details

In `WaveformDisplay::drawBackground()`, locate the grid line drawing:
```cpp
g.setColour (MLIMColours::waveformGridLine.withAlpha (0.6f));
```

Change to:
```cpp
g.setColour (MLIMColours::waveformGridLine.withAlpha (0.35f));
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

convert /tmp/mlim.png -crop 640x500+0+0 +repage /tmp/wave-mlim.png
convert /tmp/ref.png  -crop 640x500+0+0 +repage /tmp/wave-ref.png
compare -metric RMSE /tmp/wave-ref.png /tmp/wave-mlim.png /dev/null 2>&1
compare -metric RMSE /tmp/ref.png /tmp/mlim.png /dev/null 2>&1
```

## Dependencies
Requires task 379
