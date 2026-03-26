# Task 328: Revert Output Meter Width (kOutputMeterW 80 → 58)

## Description
Task 320 widened the output level meter from 58px to 80px. The RMSE remeasure (task 324)
confirmed this caused a waveform-area regression (+0.98 pp). Pixel sampling of the reference
(prol2-main-ui.jpg at 900x500) shows the level meter strip between the waveform and loudness
panel is approximately 40–50px wide. Reverting to 58px moves the waveform back to 690px wide,
which is closer to the reference (~700px estimated from the reference layout).

Reference measurement: at x=668-720 in the 900x500 comparison space, the reference shows
light blue-gray colors (#7585B4 to #4A4D5E at y=100), consistent with the output level meter
occupying only ~50px rather than the current 80px.

## Produces
None

## Consumes
None

## Relevant Files
Modify: `M-LIM/src/PluginEditor.h` — change `kOutputMeterW` from 80 to 58

## Acceptance Criteria
- [ ] Run: `cmake --build M-LIM/build --config Release -j$(nproc) 2>&1 | tail -3` → Expected: build succeeds, exit 0
- [ ] Run: `source Scripts/ui-test-helper.sh && start_app && screenshot "screenshots/task-328-after.png" && stop_app` → Expected: screenshot saved
- [ ] Visual: Output meter strip is narrower (~58px), waveform area wider; control strip layout unchanged

## Tests
None

## Technical Details
In `M-LIM/src/PluginEditor.h`, change:
```cpp
// BEFORE:
static constexpr int kOutputMeterW   = 80;

// AFTER:
static constexpr int kOutputMeterW   = 58;
```

This is the only change needed. The layout in `resized()` uses this constant directly.

## Dependencies
None
