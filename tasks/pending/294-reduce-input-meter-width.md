# Task 294: Reduce Input Level Meter Width to Match Reference Proportions

## Description
The input level meter (`inputMeter_`) is currently 30px wide (`kInputMeterW = 30` in PluginEditor.h).
The Pro-L 2 reference shows the input level meter as an extremely narrow strip — approximately 5-8px
wide at the 900px scaled output, meaning about 9-14px wide in the original 1712px reference image.

M-LIM's 30px meter (before waveform starts) vs the reference's ~8px meter means the waveform display
in M-LIM starts ~22px further right than in the reference. This structural misalignment contributes to
elevated RMSE across the left region.

**Fix**: Reduce `kInputMeterW` from 30 to **16 px**. This halves the meter width, shifting the
waveform ~14px to the left and improving layout parity. The LevelMeter channel bars (each about
`16 * 0.42 = 6.7 px`) remain narrow but readable.

Do this AFTER task 293 is complete (so the left edge has no ADVANCED strip, only the meter).

## Produces
None

## Consumes
None

## Relevant Files
Modify: `M-LIM/src/PluginEditor.h` — change `kInputMeterW` from 30 to 16
Read: `M-LIM/src/ui/LevelMeter.cpp` — check `kBarWidthRatio` (currently 0.42) and `kGapRatio`
      (currently 0.08); may need adjustment to `kBarWidthRatio = 0.44` to use more of the 16px
Read: `M-LIM/src/PluginEditor.cpp` — verify inputMeter_ setBounds call and any gain-badge x offsets

## Acceptance Criteria
- [ ] Run: `cmake --build /workspace/M-LIM/build --config Release -j$(nproc) --target MLIM_Standalone` → Expected: exits 0
- [ ] Visual: Screenshot shows input level meter as a narrow strip (≤18px) on the far left; waveform display starts no more than 18px from left edge of plugin content area
- [ ] Run: full-image RMSE compare → Expected: improvement vs pre-task baseline (< 0.255 full-image RMSE)

## Tests
None

## Technical Details
Change in `PluginEditor.h`:
```cpp
static constexpr int kInputMeterW = 16;  // was 30; reference proportions ~8px at 900px scale
```

In `LevelMeter.cpp`, with 16px total width and `kScaleWidth = 0`:
- `availW = 16`
- `barW = 16 * 0.42 = 6.72px` per channel
- `gap = 16 * 0.08 = 1.28px`
- Total: 6.72 + 1.28 + 6.72 = 14.72px (leaves ~1.28px unaccounted, fine)

If the bars look too thin at 6.7px each, adjust `kBarWidthRatio` to 0.44 in LevelMeter.cpp:
- `barW = 16 * 0.44 = 7.04px`

The gain badge position in `PluginEditor::resized()` is calculated as:
```cpp
const int badgeX = waveformDisplay_.getX() + 4;
```
This uses `waveformDisplay_.getX()` which auto-adjusts when kInputMeterW changes — no manual update needed.

## Dependencies
Requires task 293 (ADVANCED button removed from left edge first, to avoid compounding layout issues)
