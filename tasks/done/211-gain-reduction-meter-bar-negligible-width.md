# Task 211: GainReductionMeter — Bar Area Only 3px Wide at Default Component Size

## Description
`GainReductionMeter` reserves `kScaleW = 22` pixels on the right for dB scale labels. In `PluginEditor.h`, the GR meter is allocated `kGRMeterW = 25` pixels wide. After removing the scale area:

```
barArea width = 25 - 22 = 3 px
```

A 3-pixel-wide gain reduction bar is essentially invisible. The scale labels (`-3`, `-6`, `-9`, `-12`, `-18`, `-24`) also barely fit in 22px at 9pt — many will overflow or be truncated.

The reference Pro-L 2 shows a clearly visible vertical red GR bar filling the meter column. The bar should be the dominant element, with scale labels either suppressed or very narrow for this component.

Fix (two-part):
1. **Widen the GR meter** in `PluginEditor.h`: increase `kGRMeterW` from `25` to `40`. This gives `barArea = 40 - 22 = 18px`, a clearly visible bar.
2. **Tighten the scale** in `GainReductionMeter.cpp`: reduce `kScaleW` from `22` to `16` and use abbreviated labels (`0`, `3`, `6`, `9`, `12`, `18`, `24` without the leading minus sign — the context makes the direction clear, matching Pro-L 2 style). This gives `barArea = 40 - 16 = 24px`.

## Produces
None

## Consumes
None

## Relevant Files
Modify: `M-LIM/src/PluginEditor.h` — increase `kGRMeterW` from 25 to 40
Modify: `M-LIM/src/ui/GainReductionMeter.h` — reduce `kScaleW` from 22 to 16 (private constant in class)
Modify: `M-LIM/src/ui/GainReductionMeter.cpp` — update `drawScale()` label format to use unsigned integers (drop the leading minus sign), and tighten `drawScale()` label rectangle width
Read: `M-LIM/src/ui/GainReductionMeter.h` — constants, bar/scale/numeric layout
Read: `/reference-docs/reference-screenshots/prol2-main-ui.jpg` — shows GR meter column width and visible bar

## Acceptance Criteria
- [ ] Run: `cmake --build build --config Release -j$(nproc) && echo OK` → Expected: `OK`
- [ ] Run: `source Scripts/ui-test-helper.sh && start_app && screenshot "screenshots/task-196-after.png" && stop_app` → Expected: a clearly visible red GR bar (> 10px wide) with compact scale labels on the right edge, no overflow
- [ ] Run: `cd build && ctest --output-on-failure` → Expected: all tests pass

## Tests
None (layout-only change)

## Technical Details

### PluginEditor.h change
```cpp
// BEFORE:
static constexpr int kGRMeterW = 25;

// AFTER:
static constexpr int kGRMeterW = 40;
```

### GainReductionMeter.h change

```cpp
// BEFORE (line ~45 in class body):
static constexpr int kScaleW  = 22;  // width of dB scale labels

// AFTER:
static constexpr int kScaleW  = 16;  // width of dB scale labels
```

### GainReductionMeter.cpp changes

In `drawScale()`, update label format and rectangle:
```cpp
// BEFORE:
juce::String label = (mark == 0.0f) ? "0" : "-" + juce::String(juce::roundToInt(mark));
auto labelRect = juce::Rectangle<float>(scaleArea.getX() + 2.0f, y - 5.0f,
                                         scaleArea.getWidth() - 4.0f, 10.0f);

// AFTER (drop leading minus — scale direction is unambiguous in context):
juce::String label = juce::String(juce::roundToInt(mark));
auto labelRect = juce::Rectangle<float>(scaleArea.getX() + 1.0f, y - 5.0f,
                                         scaleArea.getWidth() - 2.0f, 10.0f);
```

Also use 8pt font in `drawScale()` to better fit the narrower scale column:
```cpp
g.setFont(juce::Font(8.0f));
```

## Dependencies
Requires task 201 (both modify GainReductionMeter.cpp — do colour pass first to avoid merge conflicts)
