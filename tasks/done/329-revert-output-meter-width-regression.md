# Task 329: Revert Output Meter Width Regression (Task 320)

## Description
Task 320 widened `kOutputMeterW` from 58 to 80 pixels in `PluginEditor.h`. This made the output
level meter wider, narrowing the waveform display area and shifting proportions away from the
reference. It contributed to the wave-8 RMSE regression.

The Pro-L 2 reference shows a relatively narrow output meter on the right side of the waveform.
58px is closer to the reference proportions. Reverting restores the correct layout.

## Produces
None

## Consumes
None

## Relevant Files
Modify: `src/PluginEditor.h` — revert `kOutputMeterW` from 80 back to 58

## Acceptance Criteria
- [ ] Run: `grep "kOutputMeterW" /workspace/M-LIM/src/PluginEditor.h` → Expected: `kOutputMeterW   = 58`
- [ ] Run: `cd /workspace/M-LIM && cmake --build build -j$(nproc) 2>&1 | grep -c "error:"` → Expected: `0`

## Tests
None

## Technical Details
In `src/PluginEditor.h`, change (around line 68):
```cpp
static constexpr int kOutputMeterW   = 80;
```
To:
```cpp
static constexpr int kOutputMeterW   = 58;
```

## Dependencies
None
