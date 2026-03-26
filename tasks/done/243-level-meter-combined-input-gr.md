# Task 243: Level Meter — Combine Input Level and GR in Same Vertical Strip

## Description
In Pro-L 2, the meter strip to the right of the waveform shows **input level (blue,
rising from bottom) and gain reduction (red, descending from top) overlaid in the same
vertical strip**. This creates an intuitive display where the red GR fill from the top
"eats into" the blue input level bar from the bottom.

M-LIM currently uses two separate components:
- `GainReductionMeter` (40 px wide, shows GR only with separate numeric readout at top)
- `LevelMeter` (48 px wide, shows input or output level bars separately)

These are separate components positioned adjacent to each other, which produces a
wider, less integrated meter section than the reference.

**Fix**: Extend `GainReductionMeter` to also render the input level bars (L and R
channels), creating a combined display:
1. Add `setInputLevel(float leftDB, float rightDB)` setter to `GainReductionMeter`.
2. In `GainReductionMeter::paint()`, draw the input level bars (blue, from bottom)
   BEFORE drawing the GR bar (red, from top), so GR overlays the level.
3. Use `MLIMColours::meterSafe` for the input level fill and
   `MLIMColours::gainReduction` with alpha ~0.8 for the GR fill.
4. In `PluginEditor.cpp applyMeterData()`, call
   `grMeter_.setInputLevel(inL, inR)` with the input levels.
5. In `PluginEditor.h resized()`, widen `kGRMeterW` from 40 to 50 px to give the
   combined meter more visual weight (and reduce `kOutputMeterW` from 48 to 38 px to
   compensate, keeping total side width the same).

The `inputMeter_` (left side) and `outputMeter_` (right side) layout remains unchanged
for now — this task only adds input level display inside the existing GR meter component.

## Produces
None

## Consumes
None

## Relevant Files
Modify: `src/ui/GainReductionMeter.h` — add inputLevelL_/R_ members and setInputLevel() method
Modify: `src/ui/GainReductionMeter.cpp` — draw input level bars in paint() before GR bar
Modify: `src/PluginEditor.cpp` — call grMeter_.setInputLevel() in applyMeterData()
Modify: `src/PluginEditor.h` — update kGRMeterW/kOutputMeterW constants

## Acceptance Criteria
- [ ] Run: build, play audio through plugin → Expected: GR meter strip shows blue input level rising from bottom with red GR overlay from top
- [ ] Run: with no audio → Expected: GR meter strip shows no level (blue bars at minimum), no GR (red absent)
- [ ] Run: `cd build && ctest --output-on-failure` → Expected: all tests pass

## Tests
None (visual change to existing component; no new testable logic)

## Technical Details
In `GainReductionMeter.h` add:
```cpp
void setInputLevel(float leftDB, float rightDB);
private:
    float inputLevelL_ = -96.0f;
    float inputLevelR_ = -96.0f;
    static constexpr float kInputMinDB = -30.0f;
    static constexpr float kInputMaxDB = 0.0f;
```

In `drawBar()` or a new `drawInputLevel()` helper, draw two thin vertical bars (L and R)
using input level values mapped from kInputMinDB to kInputMaxDB. The bars should use
`MLIMColours::meterSafe` fill and occupy roughly half the barArea width each.

In `PluginEditor.cpp`:
```cpp
grMeter_.setInputLevel(inL, inR);  // add after existing grMeter_ call
```

## Dependencies
None
