# Task 379: Increase Waveform Idle Fill Opacity

## Description

The waveform idle fill simulation (task 369) adds a semi-transparent blue overlay to the
lower 56% of the waveform. The current bottom alpha caps at `0.62f`.

Pixel analysis (with wave 17 state including task 376 mid-zone fill):
- Reference waveform bottom (y=380 of comparison crop): `#7A809B` (R=122, G=128, B=155)
- M-LIM waveform bottom: `#5B6B95` (R=91, G=107, B=149)

M-LIM is still ~25-30 pixel counts too dark in R and G channels at the bottom. Increasing
the idle fill alpha from `0.62f` to `0.80f` will bring the composite appearance closer
to the reference's waveform-fill-brightened bottom.

**Expected RMSE gain:** Wave region −0.2 to −0.3pp.

**Note:** This modifies `WaveformDisplay.cpp`. Task 380 (reduce grid line alpha) also
modifies `WaveformDisplay.cpp` and must be done AFTER this task to avoid file conflicts.

## Produces
None

## Consumes
None

## Relevant Files
Modify: `src/ui/WaveformDisplay.cpp` — `drawBackground()` method, idle fill bottom alpha
Read: `src/ui/Colours.h` — `inputWaveform` color (`0xCC6878A0`)

## Acceptance Criteria
- [ ] Run: build + screenshot + wave RMSE → Expected: Wave RMSE decreases from wave 17 baseline
- [ ] Run: full image RMSE → Expected: ≤ wave-17 full baseline (no regression)
- [ ] Run: visual check → Expected: lower half of waveform shows subtle blue tint; top 44% unchanged

## Tests
None

## Technical Details

In `WaveformDisplay::drawBackground()`, locate the idle fill gradient:
```cpp
juce::ColourGradient fillGrad (
    MLIMColours::inputWaveform.withAlpha (0.0f),   0.0f, fillTop,
    MLIMColours::inputWaveform.withAlpha (0.62f),  0.0f, area.getBottom(),
    false);
```

Change the bottom alpha from `0.62f` to `0.80f`:
```cpp
juce::ColourGradient fillGrad (
    MLIMColours::inputWaveform.withAlpha (0.0f),   0.0f, fillTop,
    MLIMColours::inputWaveform.withAlpha (0.80f),  0.0f, area.getBottom(),
    false);
```

If wave RMSE increases, try `0.72f` as a midpoint.

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
Requires task 376, 377
