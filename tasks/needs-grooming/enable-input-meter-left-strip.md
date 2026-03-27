# Task: Enable input meter left strip (Left zone RMSE improvement)

## Description
The input level meter is currently hidden (`inputMeter_.setVisible(false)` in PluginEditor.cpp:23).
The reference Pro-L 2 shows a visible level meter strip between the waveform and the loudness panel.

Pixel analysis of the Left zone (x=640–720 of 900×500 crop) confirms the discrepancy:
- M-LIM mid-40% average: #433D48 (R=67, G=61, B=72)
- Reference mid-40% average: #6D7189 (R=109, G=113, B=137)
- Gap: +42R, +52G, +65B — reference is significantly brighter here

The reference shows active level meter bar content in this zone (warm gradient segments with dB scale labels on the left).  M-LIM shows a narrow GR meter (12px) + dark loudness panel in this zone.

Enabling the input meter (30px wide, kInputMeterW already defined) adds a visible level bar in the left-zone area that will close this brightness gap.

## Produces
None

## Consumes
None

## Relevant Files
Modify: `M-LIM/src/PluginEditor.cpp` — remove `setVisible(false)` call (line 23), re-enable input meter in `resized()` layout
Modify: `M-LIM/src/PluginEditor.h` — kInputMeterW = 30 (already defined, no change needed)
Read: `M-LIM/src/ui/LevelMeter.cpp` — understand meter rendering for scale positioning

## Acceptance Criteria
- [ ] Run: `ls M-LIM/build/MLIM_artefacts/Release/Standalone/M-LIM` → Expected: binary exists (build passes)
- [ ] Run: screenshot + Left zone RMSE measurement (crop 80x500+640+0) → Expected: Left zone RMSE lower than 26.22% baseline
- [ ] Run: Full zone RMSE → Expected: Full RMSE ≤ 19.11% (no regression)

## Tests
None

## Technical Details
In `PluginEditor.cpp resized()`, re-add:
```cpp
inputMeter_.setBounds(bounds.removeFromLeft(kInputMeterW));
```
Place this BEFORE the GR meter removal, so order is (left→right):
  input meter → waveform → GR meter → loudness panel → output meter

Remove `inputMeter_.setVisible(false)` from the constructor (currently line 23).

The inputMeter_ is already added via `addAndMakeVisible(inputMeter_)` at line 22.

Call `inputMeter_.setShowScale(true)` to show dB labels on the right edge of the input meter.

The waveform will shrink from ~648px to ~618px but remains ~68% of the 900px total width, which is within acceptable range.

After enabling, build and measure RMSE with:
```bash
CCACHE_DIR=/build-cache/ccache cmake --build M-LIM/build --target MLIM_Standalone -j$(nproc)
source Scripts/ui-test-helper.sh && start_app 99
screenshot "task-NNN-after.png"
# Crop and measure Left zone
convert screenshots/task-NNN-after.png -crop 908x500+509+325 +repage -resize 900x500! /tmp/mlim.png
convert /reference-docs/reference-screenshots/prol2-main-ui.jpg -crop 1712x1073+97+32 +repage -resize 900x500! /tmp/ref.png
convert /tmp/mlim.png -crop 80x500+640+0 +repage /tmp/z-left.png
convert /tmp/ref.png  -crop 80x500+640+0 +repage /tmp/r-left.png
compare -metric RMSE /tmp/z-left.png /tmp/r-left.png /dev/null 2>&1
stop_app
```

## Dependencies
None
