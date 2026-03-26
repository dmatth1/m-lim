# Task 340: Revert Task 333 — Re-hide Input Level Meter

## Description
Task 333 (worker-1) removed `inputMeter_.setVisible(false)` from PluginEditor.cpp, making the
input level meter visible. This is a critical regression: task-339 RMSE measurement shows the
left meter sub-region jumped from 23.71% (task-324 baseline) to 32.70% — a 9 percentage-point
regression. The full-image RMSE is now 25.74% vs. the 21.97% best (task-317).

The dark idle input meter bar does NOT match the reference (which shows active waveform content
filling that region). Re-hiding the input meter allows the waveform display gradient to fill
the left edge area, which better matches the reference's visual appearance.

**Fix**: Re-add `inputMeter_.setVisible(false)` in PluginEditor.cpp after `addAndMakeVisible(inputMeter_)`.

## Produces
None

## Consumes
None

## Relevant Files
Modify: `M-LIM/src/PluginEditor.cpp` — re-add `inputMeter_.setVisible(false)` at line ~22

## Acceptance Criteria
- [ ] Run: `cmake --build M-LIM/build -j$(nproc) 2>&1 | grep -c "error:"` → Expected: `0`
- [ ] Run: visual inspection — left side of waveform should show the gradient, not a dark meter bar
- [ ] The `inputMeter_` member should still exist and be updated by timerCallback (just not rendered)

## Tests
None

## Technical Details
In `M-LIM/src/PluginEditor.cpp`, in the constructor, find:
```cpp
addAndMakeVisible (inputMeter_);
```
Change to:
```cpp
addAndMakeVisible (inputMeter_);
inputMeter_.setVisible (false);
```

The `inputMeter_` component still occupies its layout bounds (kInputMeterW = 30px strip on left)
and still receives data updates in `applyMeterData()`. It is just not rendered. This keeps the
waveform display starting at x=30 (the same as before task 333), maintaining the layout.

Note: Do NOT remove `inputMeter_.setBounds(...)` from the `resized()` method — the bounds must
still be set so the component occupies its 30px strip and the waveform fills the rest.

## Dependencies
None
