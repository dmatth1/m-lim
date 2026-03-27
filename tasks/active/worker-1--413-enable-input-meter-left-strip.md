# Task 413: Enable Input Meter Left Strip (Left Zone RMSE Improvement)

## Description
The input level meter is currently hidden (`inputMeter_.setVisible(false)` in PluginEditor.cpp).
The reference Pro-L 2 shows a visible level meter strip between the waveform and the loudness panel.

Pixel analysis of the Left zone (x=640–720 of 900×500 crop) confirms the discrepancy:
- M-LIM mid-40% average: #433D48 (R=67, G=61, B=72)
- Reference mid-40% average: #6D7189 (R=109, G=113, B=137)
- Gap: +42R, +52G, +65B — reference is significantly brighter here

Enabling the input meter (30px wide, kInputMeterW already defined) adds a visible level bar
in the left-zone area that will close this brightness gap.

## Produces
None

## Consumes
None

## Relevant Files
Modify: `src/PluginEditor.cpp` — remove `setVisible(false)` call, re-enable input meter in `resized()` layout
Read:   `src/PluginEditor.h` — kInputMeterW = 30 (already defined, confirm value)
Read:   `src/ui/LevelMeter.cpp` — understand meter rendering for scale positioning

## Acceptance Criteria
- [ ] Run: `cmake --build build --target MLIM_Standalone_Standalone -j$(nproc)` → Expected: compiles without error
- [ ] Run: screenshot + Left zone RMSE (crop 80x500+640+0) → Expected: Left zone RMSE < 26.22% baseline
- [ ] Run: Full RMSE → Expected: Full RMSE ≤ 19.11% (no regression)

## Tests
None

## Technical Details
In `PluginEditor.cpp resized()`, re-add:
```cpp
inputMeter_.setBounds(bounds.removeFromLeft(kInputMeterW));
```
Place this BEFORE removing the GR meter from the right, so order left→right is:
  input meter → waveform → GR meter → loudness panel → output meter

Remove `inputMeter_.setVisible(false)` from the constructor.

The `inputMeter_` is already added via `addAndMakeVisible(inputMeter_)`. Also call:
```cpp
inputMeter_.setShowScale(true);
```
to show dB labels on the right edge of the input meter.

The waveform will shrink from ~648px to ~618px but remains ~68% of total width.

Left zone RMSE measurement:
```bash
convert /tmp/task-mlim.png -crop 80x500+640+0 +repage /tmp/z-left.png
convert /tmp/task-ref.png  -crop 80x500+640+0 +repage /tmp/r-left.png
compare -metric RMSE /tmp/r-left.png /tmp/z-left.png /dev/null 2>&1
```

## Dependencies
None
