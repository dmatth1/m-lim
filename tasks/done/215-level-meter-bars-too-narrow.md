# Task 215: LevelMeter — Channel Bars Only 3.4px Wide at Current Component Size

## Description
`LevelMeter::paint()` computes bar width as:

```cpp
const float availW = bounds.getWidth() - (float)kScaleWidth;  // kScaleWidth = 22
const float barW   = availW * kBarWidthRatio;                  // kBarWidthRatio = 0.42
```

With `kInputMeterW = kOutputMeterW = 30` (set in `PluginEditor.h`):
```
availW = 30 - 22 = 8 px
barW   = 8 × 0.42 = 3.4 px  (per channel)
```

Two bars of 3.4px each are barely visible — essentially a pair of thin lines. The reference Pro-L 2 shows clearly visible stereo bars in the input and output level meters.

Additionally, the 22px scale column is just wide enough to show `-12` (3 chars) but tight for labels at 9pt with 5px start offset, and may clip on smaller fonts.

Fix:
1. Widen the input and output meters: increase `kInputMeterW` and `kOutputMeterW` from 30 to 48 in `PluginEditor.h`. This gives `availW = 48 - 22 = 26px`, `barW = 26 × 0.42 ≈ 11px` per channel — clearly visible.
2. Optionally reduce `kScaleWidth` in `LevelMeter.cpp` from 22 to 18 to give each bar a bit more room (≈ 12.6px) while keeping all scale labels legible.

## Produces
None

## Consumes
None

## Relevant Files
Modify: `M-LIM/src/PluginEditor.h` — increase `kInputMeterW` and `kOutputMeterW` from 30 to 48
Modify: `M-LIM/src/ui/LevelMeter.cpp` — optionally reduce `kScaleWidth` from 22 to 18 to give bars more room
Read: `M-LIM/src/ui/LevelMeter.h` — layout constants and bar structure
Read: `/reference-docs/reference-screenshots/prol2-main-ui.jpg` — reference for visible stereo bars in side level meters

## Acceptance Criteria
- [ ] Run: `cmake --build build --config Release -j$(nproc) && echo OK` → Expected: `OK`
- [ ] Run: `source Scripts/ui-test-helper.sh && start_app && screenshot "screenshots/task-197-after.png" && stop_app` → Expected: input and output level meter bars are clearly visible (each channel bar visually distinct, > 8px wide), dB scale labels readable within the component bounds
- [ ] Run: `cd build && ctest --output-on-failure` → Expected: all tests pass

## Tests
None (layout-only change)

## Technical Details

### PluginEditor.h
```cpp
// BEFORE:
static constexpr int kInputMeterW    = 30;
static constexpr int kOutputMeterW   = 30;

// AFTER:
static constexpr int kInputMeterW    = 48;
static constexpr int kOutputMeterW   = 48;
```

### LevelMeter.cpp (optional improvement)
```cpp
// BEFORE:
constexpr int kScaleWidth = 22;

// AFTER:
constexpr int kScaleWidth = 18;
```

With 48px total and 18px scale: `availW = 30px`, `barW = 30 × 0.42 = 12.6px` — clean, visible bars.

Note: widening the meters slightly reduces the waveform display width (from ~675px to ~639px at 900px total). The waveform still occupies ~71% of total width, well within the target ~70%.

## Dependencies
Requires task 211 (both modify PluginEditor.h layout constants — do GR meter width first)
