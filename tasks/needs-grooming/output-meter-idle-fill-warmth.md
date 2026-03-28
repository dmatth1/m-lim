# Task: Output Level Meter — Add Idle Warm Fill to Match Reference

## Description
The output level meter (rightmost 100px strip, rendered by `LevelMeter` in `PluginEditor`) shows at idle as very dark (sampled pixel: RGB 58, 61, 83). The reference Pro-L 2 shows output meters with active signal, appearing with warm/green-blue fill colors in the lower portion.

The output meter uses the same `LevelMeter` component as the input meter, so if the idle gradient redesign task (level-meter-idle-gradient-active-simulation) is implemented, the output meter will also benefit.

However, the output meter is configured with `outputMeter_.setShowScale(false)` (no scale labels), and is positioned as the rightmost strip. The reference shows the output meters as paired stereo bars with visible activity.

**Additional specific fix**: The `LevelMeter::drawChannel()` for the output meter receives only `levelDB = kMinDB` at idle (no audio). The idle gradient should therefore simulate the output meter appearance at typical output level (-0.1 dBTP), which means the meter should appear ~98% full with only the topmost segment dark.

Since the output meter idle gradient uses the same code path as the input meter, after the active-simulation idle gradient task is implemented, the output meter will show the simulated fill.

**Additional standalone fix for this task**: Change the `LevelMeter` to accept an optional "simulated idle level" parameter, defaulting to -6 dBFS, but configurable. For the output meter, set to -0.5 dBFS (typical near-ceiling output).

OR (simpler alternative):
- In `PluginEditor`, after constructing `outputMeter_`, call a method to set a higher simulated idle fill level, e.g., `outputMeter_.setIdleSimulationLevel(-1.0f)`.
- This parameter controls the "cutoff" point where the idle gradient transitions from warm fill to dark background.

## Produces
None

## Consumes
None

## Relevant Files
Modify: `src/ui/LevelMeter.h` — add `setIdleSimulationLevel(float dBFS)` method and `idleSimLevel_` member
Modify: `src/ui/LevelMeter.cpp` — use `idleSimLevel_` in the idle gradient to compute the fill cutoff position
Modify: `src/PluginEditor.cpp` — call `outputMeter_.setIdleSimulationLevel(-0.5f)` in constructor

## Acceptance Criteria
- [ ] Run: `cmake --build build --target MLIM_Standalone -j$(nproc)` → Expected: build succeeds, exit 0
- [ ] Run: launch app idle, capture right panel → Expected: output meter shows nearly full appearance (98% filled with warm/blue gradient) matching reference output meter appearance

## Tests
None

## Technical Details
In `LevelMeter.h`, add:
```cpp
void setIdleSimulationLevel (float dBFS) noexcept { idleSimLevel_ = dBFS; }
float idleSimLevel_ = -6.0f;  // default: simulate audio at -6 dBFS
```

In `LevelMeter.cpp::drawChannel()`, use `idleSimLevel_` to compute `simFillTop`:
```cpp
const float simNorm = dbToNorm (idleSimLevel_);
const float simFillTop = barTop + barH * (1.0f - simNorm);  // Y position of simulated fill line
```

This gives:
- idleSimLevel_ = -6.0f → simNorm = 0.90 → simFillTop = barTop + barH * 0.10 (10% from top dark, 90% filled)
- idleSimLevel_ = -0.5f → simNorm = 0.9917 → simFillTop = barTop + barH * 0.0083 (~1% from top dark, 99% filled)

Expected RMSE improvement: ~0.5-1.0pp for Right zone, ~0.3pp for Full.

## Dependencies
Requires level-meter-idle-gradient-active-simulation task (implements the base idle simulation gradient)
