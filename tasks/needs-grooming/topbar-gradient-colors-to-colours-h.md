# Task: Move TopBar Gradient Colours to Colours.h Constants

## Description
`TopBar.cpp` paint() (line 84-85) uses hardcoded hex colour values for the top bar gradient:

```cpp
juce::ColourGradient bg (juce::Colour (0xff4A4650), 0.0f, 0.0f,
                         juce::Colour (0xff3C3842), 0.0f, bounds.getHeight(), false);
```

All other components use `MLIMColours::` constants from `Colours.h` for their gradients (controlStripTop/Bottom, loudnessPanelBackground, displayGradientTop/Bottom). The TopBar should follow the same pattern for consistency and easy tuning.

## Produces
None

## Consumes
None

## Relevant Files
Modify: `M-LIM/src/ui/Colours.h` — add `topBarGradientTop` and `topBarGradientBottom` constants
Modify: `M-LIM/src/ui/TopBar.cpp` — replace hardcoded hex values with the new constants

## Acceptance Criteria
- [ ] Run: `grep -n "0xff4A4650\|0xff3C3842" src/ui/TopBar.cpp` → Expected: no output
- [ ] Run: `cmake --build build --target MLIM_Standalone_Standalone -j$(nproc) 2>&1 | tail -3` → Expected: build succeeds

## Tests
None

## Technical Details
Add to Colours.h:
```cpp
const juce::Colour topBarGradientTop    { 0xff4A4650 };
const juce::Colour topBarGradientBottom { 0xff3C3842 };
```

## Dependencies
None
