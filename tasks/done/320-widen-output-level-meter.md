# Task 320: Widen Output Level Meter for Better Panel Proportions

## Description
The output level meter (`kOutputMeterW = 58px`) is narrower than Pro-L 2's output level bars.
In Pro-L 2, the stereo output level bars take up proportionally more of the right panel,
giving each channel bar more visual weight.

Increase `kOutputMeterW` from 58 to 80px. The loudness panel width stays at 140px.
The extra 22px come from the waveform display area (waveform reduces from 690px to 668px).

## Produces
None

## Consumes
None

## Relevant Files
Modify: `src/PluginEditor.h` — change `kOutputMeterW` from 58 to 80

## Acceptance Criteria
- [ ] Run: `cd /workspace/M-LIM && cmake --build build -j$(nproc) 2>&1 | grep -c "error:"` → Expected: `0`
- [ ] Run: launch M-LIM standalone and visually verify right panel — output meter bars should be wider (each channel bar ~26px instead of ~17px)

## Tests
None

## Technical Details
In `src/PluginEditor.h`, line ~68:
```cpp
static constexpr int kOutputMeterW = 58;
```
Change to:
```cpp
static constexpr int kOutputMeterW = 80;
```

The LevelMeter layout is driven by `kBarWidthRatio = 0.46f` and `kGapRatio = 0.08f` in
`LevelMeter.cpp`. With 80px width and kScaleWidth=20:
- availW = 80 - 20 = 60px
- barW = 60 × 0.46 = 27.6px per channel ✓

No other changes are needed — LevelMeter auto-scales to its assigned bounds.

## Dependencies
None
