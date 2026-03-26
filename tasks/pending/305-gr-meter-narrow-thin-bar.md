# Task 305: GR Meter — Narrow to Thin Bar, Remove Internal Scale

## Description
The `grMeter_` component is currently 50px wide (`kGRMeterW = 50`) with a 16px internal scale strip showing values "0, 3, 6, 9, 12, 18, 24". This creates a visually prominent element in the right panel that does NOT match the reference.

In Pro-L 2 (reference frames v1-0030, v1-0040), the gain reduction indicator in the right panel is a very **narrow thin bar** (~10–12px wide) immediately to the right of the waveform edge, with no separate text scale. The GR is primarily communicated via the waveform overlay (red fill from the top), not via a wide sidebar bar.

Reducing the GR meter width and removing its internal scale will:
- Correct the right-panel proportions to match the reference
- Remove the misleading "3, 6, 9, 12, 18" numbers that appear where the output level scale should be
- Give the output level meter and loudness panel more accurate relative proportions

## Fix

1. **`PluginEditor.h`**: Change `kGRMeterW` from `50` to `12`.
2. **`GainReductionMeter.h`**: Change `kScaleW` from `16` to `0` and `kNumericH` from `28` to `16`.
3. **`GainReductionMeter.cpp`**: Add early-return guard at top of `drawScale()`:
   ```cpp
   if (kScaleW <= 0) return;
   ```
4. **`GainReductionMeter.cpp`**: In `drawNumeric()`, reduce font sizes to fit the narrower/shorter header:
   - Current GR value: use `kFontSizeSmall` (9pt) instead of `kFontSizeLarge` (11pt bold)
   - Peak GR value: use 8pt instead of `kFontSizeSmall`
5. **`GainReductionMeter.cpp`**: In `peakLabelArea()`, update to match new `kNumericH = 16`.

## Produces
None

## Consumes
None

## Relevant Files
Modify: `M-LIM/src/PluginEditor.h` — change `kGRMeterW` from `50` to `12`
Modify: `M-LIM/src/ui/GainReductionMeter.h` — change `kScaleW` to `0`, `kNumericH` to `16`
Modify: `M-LIM/src/ui/GainReductionMeter.cpp` — add guard in `drawScale()`, reduce font sizes in `drawNumeric()`

## Acceptance Criteria
- [ ] Run: `cmake --build M-LIM/build --config Release -j$(nproc) 2>&1 | tail -5` → Expected: build succeeds, exit 0
- [ ] Run: launch app, take screenshot → Expected: GR meter is a narrow thin bar (~12px) at the left edge of the right panel; no "3, 6, 9, 12" scale labels visible in the meter column
- [ ] Run: `source Scripts/ui-test-helper.sh && start_app && screenshot screenshots/task-305-after.png && stop_app` → Expected: screenshot saved
- [ ] Run: compare to reference → Expected: right-panel RMSE improves compared to task-299 baseline of 0.2960

## Tests
None

## Technical Details
- `PluginEditor.h` line ~67: `kGRMeterW = 50` → `kGRMeterW = 12`
- `GainReductionMeter.h` line ~45: `kScaleW = 16` → `kScaleW = 0`
- `GainReductionMeter.h` line ~46: `kNumericH = 28` → `kNumericH = 16`
- In `drawNumeric()`: scale down font from `kFontSizeLarge` to `kFontSizeSmall` for the current GR label
- The 12px width with 0px scale means the full width is used for the bar
- The bar top-down fill (red segments for gain reduction) remains unchanged

## Dependencies
None (can run in parallel with task 304)
