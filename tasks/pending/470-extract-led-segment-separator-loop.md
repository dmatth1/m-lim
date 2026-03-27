# Task 470: Extract Shared LED Segment Separator Drawing Loop

## Description
`GainReductionMeter::drawBar()` and `LevelMeter::drawChannel()` contain an identical loop pattern for drawing LED-style segment separators:

In GainReductionMeter.cpp (~line 107):
```cpp
for (float sy = barTop; sy < barTop + barH; sy += kSegH + kSegGap)
    g.fillRect (barArea.getX(), sy + kSegH, barArea.getWidth(), kSegGap);
```

In LevelMeter.cpp (~line 83):
```cpp
for (float sy = barTop; sy < barTop + barH; sy += kSegH + kSegGap)
    g.fillRect (bar.getX(), sy + kSegH, bar.getWidth(), kSegGap);
```

This is identical logic — only the rect variable name differs. Extract to a shared helper function.

## Produces
None

## Consumes
None

## Relevant Files
Read: `M-LIM/src/ui/GainReductionMeter.cpp` — locate the segment separator loop
Read: `M-LIM/src/ui/LevelMeter.cpp` — locate the identical loop
Modify: `M-LIM/src/ui/Colours.h` or create a small `M-LIM/src/ui/MeterUtils.h` — add inline helper
Modify: `M-LIM/src/ui/GainReductionMeter.cpp` — use the shared helper
Modify: `M-LIM/src/ui/LevelMeter.cpp` — use the shared helper

## Acceptance Criteria
- [ ] Run: `cmake --build build --target MLIM_Standalone_Standalone -j$(nproc) 2>&1 | tail -3` → Expected: build succeeds

## Tests
None

## Technical Details
Helper signature:
```cpp
inline void drawSegmentSeparators(juce::Graphics& g,
                                   float x, float barTop, float barH, float barW,
                                   float segH, float segGap, juce::Colour sepColour)
```

Could live in Colours.h (alongside existing shared constants) or in a new `MeterUtils.h`. Prefer Colours.h to avoid adding a new file.

## Dependencies
None
