# Task 415: Level Meter Active Fill — Shift Mid-Range Color Warmer (Right Zone RMSE)

## Description
The level meter active fill gradient (drawn when audio level > kMinDB) uses
`meterSafe.brighter(0.15f)` at the `warnBot` position (-12 dBFS). The `meterSafe` color
is a cool steel-blue (#6879A0), so `brighter(0.15)` remains blue-shifted.

This results in the mid-range of the meter (when active) showing a blue-cool gradient where
the reference shows amber/warm segments.

Additionally, the active fill's bottom colour uses `meterSafe.darker(0.3f)` which produces
a very dark blue-gray at the low end of the fill — the reference shows more visible
cool-blue segments at low levels rather than near-black.

Wave-22 baseline Right zone RMSE = 23.50%.

## Produces
None

## Consumes
None

## Relevant Files
Modify: `src/ui/LevelMeter.cpp` — `drawChannel()` active fill gradient (lines ~132–145)
Read:   `src/ui/Colours.h` — meterSafe, meterWarning, grMeterLow constants

## Acceptance Criteria
- [ ] Run: `cmake --build build --target MLIM_Standalone_Standalone -j$(nproc)` → Expected: compiles without error
- [ ] Run: Right zone RMSE (crop 180x500+720+0) → Expected: Right RMSE < 23.50% baseline
- [ ] Run: Full RMSE → Expected: Full ≤ 19.11% (no regression)

## Tests
None

## Technical Details
In `LevelMeter.cpp drawChannel()`, active fill gradient (lines ~132–145):

**Current:**
```cpp
juce::ColourGradient gradient (
    MLIMColours::meterDanger,             0.0f, barTop,
    MLIMColours::meterSafe.darker(0.3f),  0.0f, barTop + barH,
    false);
gradient.addColour((dangerBot - barTop) / barH, MLIMColours::meterWarning);
gradient.addColour((warnBot   - barTop) / barH, MLIMColours::meterSafe.brighter(0.15f));
```

**Proposed adjustment:**
```cpp
juce::ColourGradient gradient (
    MLIMColours::meterDanger,              0.0f, barTop,
    MLIMColours::meterSafe.darker(0.1f),   0.0f, barTop + barH,   // lighter bottom (was 0.3f)
    false);
gradient.addColour((dangerBot - barTop) / barH, MLIMColours::meterWarning);
gradient.addColour((warnBot   - barTop) / barH, MLIMColours::grMeterLow.withAlpha(0.8f));  // warmer amber (was cool blue)
```

Or alternatively use `MLIMColours::meterSafe.withAlpha(1.0f)` as the bottom colour
for more visible low-end, and use `grMeterLow` (#E8C840 warm yellow) at `warnBot`
for a warmer mid-range.

Measure the right zone RMSE at each step. Target: warmer mid-zone color temperature to
match the reference's amber tone at ~-6 to -12 dBFS.

Right zone RMSE measurement:
```bash
convert /tmp/task-mlim.png -crop 180x500+720+0 +repage /tmp/cur-right.png
convert /tmp/task-ref.png  -crop 180x500+720+0 +repage /tmp/ref-right.png
compare -metric RMSE /tmp/ref-right.png /tmp/cur-right.png /dev/null 2>&1
```

## Dependencies
None
