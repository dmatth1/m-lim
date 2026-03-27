# Task: Level meter active fill — shift mid-range color warmer (Right zone RMSE)

## Description
The level meter active fill gradient (drawn when audio level > kMinDB) uses
`meterSafe.brighter(0.15f)` at the warmExtY position (-12 dBFS). The `meterSafe` color
is a cool steel-blue (#6879A0), so `brighter(0.15)` remains blue-shifted.

This results in the mid-range of the meter (when active) showing a blue-cool
gradient where the reference shows amber/warm segments.

Additionally, the active fill's bottom colour uses `meterSafe.darker(0.3f)` which
produces a very dark blue-gray at the low end of the fill — the reference shows
more visible cool-blue segments at low levels rather than near-black.

Adjustments to active fill gradient in `drawChannel()`:
1. Replace the `warnBot` colour stop with `meterWarning` at that position (already done)
2. Replace the `meterSafe.brighter(0.15f)` at warnBot with a warmer transition
3. Lighten the bottom active fill colour slightly for visible low-level segments

## Produces
None

## Consumes
None

## Relevant Files
Modify: `M-LIM/src/ui/LevelMeter.cpp` — `drawChannel()` active fill gradient (lines 132–138)
Read: `M-LIM/src/ui/Colours.h` — meterSafe, meterWarning, grMeterLow constants

## Acceptance Criteria
- [ ] Run: build → Expected: compiles without error
- [ ] Run: Right zone RMSE → Expected: Right RMSE < 23.50% baseline
- [ ] Run: Full RMSE → Expected: Full ≤ 19.11% (no regression)

## Tests
None

## Technical Details
In `LevelMeter.cpp drawChannel()`, the active fill gradient (lines ~132–145):

**Current:**
```cpp
juce::ColourGradient gradient (
    MLIMColours::meterDanger,             0.0f, barTop,
    MLIMColours::meterSafe.darker(0.3f),  0.0f, barTop + barH,
    false);
gradient.addColour((dangerBot - barTop) / barH, MLIMColours::meterWarning);
gradient.addColour((warnBot   - barTop) / barH, MLIMColours::meterSafe.brighter(0.15f));
```

The `meterSafe.brighter(0.15f)` stop at `warnBot` creates a cool blue transition.
The bottom `meterSafe.darker(0.3f)` is quite dark (#3D4A65).

**Proposed adjustment:**
```cpp
// At warnBot: use a warmer amber-to-blue transition
gradient.addColour((warnBot - barTop) / barH, MLIMColours::grMeterLow.withAlpha(0.8f));
// Bottom: lighten slightly for visible low segments
// meterSafe.darker(0.1f) instead of .darker(0.3f)
juce::ColourGradient gradient (
    MLIMColours::meterDanger,              0.0f, barTop,
    MLIMColours::meterSafe.darker(0.1f),  0.0f, barTop + barH,   // lighter bottom
    false);
```

Or alternatively use `MLIMColours::meterSafe.withAlpha(1.0f)` as the bottom colour
for a more visible low-end, and use `grMeterLow` (#E8C840 warm yellow) at warnBot
for a warmer mid-range.

Measure right zone RMSE at each step. The target is warmer mid-zone color temperature
to match the reference's amber tone at ~-6 to -12 dBFS.

## Dependencies
None
