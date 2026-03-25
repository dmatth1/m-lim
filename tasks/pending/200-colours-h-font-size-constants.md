# Task 200: Add Font Size Constants to Colours.h

## Description
The literal font sizes `9.0f`, `10.0f`, and `11.0f` appear approximately 30 times across at least 8 UI source files:
- `WaveformDisplay.cpp`
- `LevelMeter.cpp`
- `GainReductionMeter.cpp`
- `LoudnessPanel.cpp`
- `RotaryKnob.cpp`
- `ControlStrip.cpp`
- `TopBar.cpp`
- `AlgorithmSelector.cpp`

This makes it impossible to change the UI font scale without a grep-and-replace hunt. Add three named constants to `Colours.h` in the `MLIMColours` namespace:

```cpp
constexpr float kFontSizeSmall   = 9.0f;
constexpr float kFontSizeMedium  = 10.0f;
constexpr float kFontSizeLarge   = 11.0f;
```

Then replace every hardcoded font size literal in the UI files with the appropriate constant.

## Produces
None

## Consumes
None

## Relevant Files
Modify: `M-LIM/src/ui/Colours.h` — add three constexpr font size constants
Modify: `M-LIM/src/ui/WaveformDisplay.cpp` — replace literal font sizes
Modify: `M-LIM/src/ui/LevelMeter.cpp` — replace literal font sizes
Modify: `M-LIM/src/ui/GainReductionMeter.cpp` — replace literal font sizes
Modify: `M-LIM/src/ui/LoudnessPanel.cpp` — replace literal font sizes
Modify: `M-LIM/src/ui/RotaryKnob.cpp` — replace literal font sizes
Modify: `M-LIM/src/ui/ControlStrip.cpp` — replace literal font sizes
Modify: `M-LIM/src/ui/TopBar.cpp` — replace literal font sizes

## Acceptance Criteria
- [ ] Run: `grep -rn "withHeight(9\.\|withHeight(10\.\|withHeight(11\." M-LIM/src/ui/` → Expected: zero results (all replaced)
- [ ] Run: `grep -n "kFontSize" M-LIM/src/ui/Colours.h` → Expected: three constant definitions found
- [ ] Run: `cmake --build build -j4 2>&1 | tail -5` → Expected: build succeeds, exit 0

## Tests
None

## Technical Details
- Do not change the numeric values — this is a pure rename refactor, not a visual change.
- Some files may construct `juce::Font` with `juce::FontOptions().withHeight(9.0f)` — replace with `juce::FontOptions().withHeight(MLIMColours::kFontSizeSmall)`.
- Include `Colours.h` in any file that doesn't already include it.

## Dependencies
None
