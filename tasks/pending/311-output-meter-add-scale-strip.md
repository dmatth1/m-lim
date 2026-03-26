# Task 311: Output Level Meter — Add dB Scale Strip

## Description
The output level meter (`outputMeter_`, right of the waveform) currently has no dB scale labels
because `kScaleWidth = 0` in `LevelMeter.h`. The reference (Pro-L 2 video frames v1-0030, v1-0040)
clearly shows a vertical scale strip to the RIGHT of the stereo bars, displaying values:
−3, −6, −9, −12, −15, −18, −21, −24, −27. This missing scale is a major visual difference
driving the right-panel RMSE (currently 29.60%).

The `drawScale()` method already exists in `LevelMeter.cpp` and works correctly — it just needs
`kScaleWidth` set to a non-zero value so the scale area is reserved and drawn.

## Fix

1. In `LevelMeter.h`: change `kScaleWidth` from `0` to `20`.
2. In `PluginEditor.h`: change `kOutputMeterW` from `38` to `58` (adds the 20px scale, preserving
   the same bar widths since `availW = total - kScaleWidth` stays at 38px).
3. Verify `drawScale()` in `LevelMeter.cpp` draws correctly at the new width.

## Produces
None

## Consumes
None

## Relevant Files
Modify: `M-LIM/src/ui/LevelMeter.h` — change `kScaleWidth` constant from `0` to `20`
Modify: `M-LIM/src/PluginEditor.h` — change `kOutputMeterW` from `38` to `58`
Read: `M-LIM/src/ui/LevelMeter.cpp` — verify `drawScale()` handles the new 20px strip correctly

## Acceptance Criteria
- [ ] Run: `cmake --build M-LIM/build --config Release -j$(nproc) 2>&1 | tail -5` → Expected: build succeeds, exit 0
- [ ] Run: `source Scripts/ui-test-helper.sh && start_app && screenshot screenshots/task-311-after.png && stop_app` → Expected: screenshot saved
- [ ] Visual inspect: dB scale labels (−3, −6, −9, …, −27) visible to the right of the two stereo level bars

## Tests
None

## Technical Details
- `LevelMeter.h` line ~12: `constexpr int kScaleWidth = 0;` → change to `constexpr int kScaleWidth = 20;`
- `PluginEditor.h` line ~67: `static constexpr int kOutputMeterW = 38;` → change to `static constexpr int kOutputMeterW = 58;`
- With these changes, bar widths remain: `availW = 58 - 20 = 38`; `barW = 38 * 0.46 ≈ 17.5px` per channel (unchanged)
- The scale labels in `drawScale()` use `kMeterGridDB` (defined in `Colours.h`): 0, −3, −6, …, −30 dB.

## Dependencies
None
