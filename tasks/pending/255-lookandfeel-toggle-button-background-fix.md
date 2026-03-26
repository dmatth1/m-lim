# Task 255: LookAndFeel drawButtonBackground Ignores Per-Component Toggle Color

## Description
`MLIMLookAndFeel::drawButtonBackground()` hardcodes `MLIMColours::accentBlue.withAlpha(0.8f)`
for any toggled-on button, regardless of the per-component color set via
`setColour(TextButton::buttonOnColourId, ...)`. This makes the "True Peak Limiting"
button (and any other status-bar toggle) render as a bright solid blue box even after
task 244 set its ON-state colour to `accentBlue.withAlpha(0.15f)`.

**Root cause** (`src/ui/LookAndFeel.cpp` ~line 152):
```cpp
if (button.getToggleState())
    fillColour = MLIMColours::accentBlue.withAlpha (0.8f);  // hardcoded — ignores setColour()
```

The `backgroundColour` parameter passed by JUCE already contains the resolved
per-component colour; the function signature marks it as unused (`/*backgroundColour*/`).

**Fix**: Replace the hardcoded colour with the resolved `backgroundColour` parameter:
```cpp
if (button.getToggleState())
    fillColour = backgroundColour;  // respects per-component setColour(buttonOnColourId, ...)
```

This allows:
- "True Peak Limiting" to show as a very subtle tint (`accentBlue.withAlpha(0.15f)`) when active
- Regular action buttons (which set `buttonOnColourId` to a solid colour) to continue working correctly
- Any future button to control its own toggle appearance via `setColour()`

## Produces
None

## Consumes
None

## Relevant Files
Modify: `src/ui/LookAndFeel.cpp` — replace hardcoded accentBlue in drawButtonBackground toggle branch
Read:   `src/ui/Colours.h` — MLIMColours::accentBlue constant
Read:   `src/ui/ControlStrip.cpp` — confirm truePeakLimitingButton_ sets buttonOnColourId to subtle tint

## Acceptance Criteria
- [ ] Run: build, launch standalone → Expected: "True Peak Limiting" in status bar shows as bright green text on a nearly-transparent background (no solid blue fill) when enabled
- [ ] Run: build, launch standalone → Expected: "ADVANCED >>" button still shows solid blue tint when toggled (it uses `accentBlue.withAlpha(0.7f)`)
- [ ] Run: `cd /workspace/M-LIM/build && ctest --output-on-failure` → Expected: all tests pass

## Tests
None (visual fix; no new testable logic)

## Technical Details
In `src/ui/LookAndFeel.cpp`, `drawButtonBackground()`:
```cpp
// BEFORE:
if (button.getToggleState())
    fillColour = MLIMColours::accentBlue.withAlpha (0.8f);

// AFTER:
if (button.getToggleState())
    fillColour = backgroundColour;
```

Also remove the `/*backgroundColour*/` comment from the parameter to make it used:
```cpp
void MLIMLookAndFeel::drawButtonBackground (juce::Graphics& g,
                                             juce::Button& button,
                                             const juce::Colour& backgroundColour,
                                             bool shouldDrawButtonAsHighlighted,
                                             bool shouldDrawButtonAsDown)
```

## Dependencies
None
