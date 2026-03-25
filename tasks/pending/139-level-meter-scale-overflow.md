# Task 139: Fix Level Meter Scale Labels Overflowing Component Bounds

## Description
The input and output level meters are 20px wide (`kInputMeterW = 20`, `kOutputMeterW = 20` in `PluginEditor.h`), but `LevelMeter.cpp` reserves `kScaleWidth = 22` pixels for scale labels. This means `availW = 20 - 22 = -2`, resulting in:
1. The two bar channels have negative computed width (invisible bars).
2. Scale labels start at `scaleX = bounds.getRight() - 22 = -2` relative to the meter bounds, rendering 2px outside the component — overflowing into the adjacent waveform display and GR meter.

Visual symptom: dB scale numbers (-6, -9, -12, …) appear floating on the left edge of the waveform display and the right edge of the GR meter, not from any intended component.

Fix: Widen the input and output level meters to 30px each (`kInputMeterW = 30`, `kOutputMeterW = 30`) so scale labels fit within bounds. Alternatively, suppress the scale in `LevelMeter::drawScale()` when `getWidth() < kScaleWidth + 4` (add a guard and draw nothing).

The Pro-L 2 reference shows side level meters with visible stereo bars; a 30px width gives `availW = 8px`, enough for two narrow bars at `kBarWidthRatio = 0.42`.

## Produces
None

## Consumes
None

## Relevant Files
Modify: `src/PluginEditor.h` — increase `kInputMeterW` and `kOutputMeterW` from 20 to 30
Modify: `src/ui/LevelMeter.cpp` — optionally guard `drawScale()` with a width check
Read: `src/ui/LevelMeter.h` — understand kScaleWidth and meter structure

## Acceptance Criteria
- [ ] Run: `source Scripts/ui-test-helper.sh && start_app && screenshot "task-139-after.png" && stop_app` → Expected: screenshot shows no dB scale numbers overflowing into waveform display area; input and output meter bars are visible (non-zero width)
- [ ] Run: `cmake --build build -j$(nproc) 2>&1 | grep -c error` → Expected: `0`

## Tests
None

## Technical Details
- `src/PluginEditor.h:66`: `static constexpr int kInputMeterW  = 20;` → change to `30`
- `src/PluginEditor.h:68`: `static constexpr int kOutputMeterW = 20;` → change to `30`
- Or add to `LevelMeter::drawScale()`: `if (getWidth() <= (int)kScaleWidth) return;`

## Dependencies
None
