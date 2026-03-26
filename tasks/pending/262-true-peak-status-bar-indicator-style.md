# Task 262: Status Bar — True Peak Limiting Green Border Indicator

## Description
The "True Peak Limiting" toggle button in the status bar currently renders with a subtle blue
background fill (`accentBlue.withAlpha(0.15f)`) when toggled on. The FabFilter Pro-L 2
reference shows this as a plain text label with a **thin green horizontal bar drawn above the
text** (~2 px tall, full button width), with no background fill change. When inactive, it shows
as dim gray text with no indicator.

**Current behaviour:**
- Toggle ON: subtle blue/teal background + bright green text
- Toggle OFF: transparent background + gray text

**Target behaviour (matching reference):**
- Toggle ON: transparent background + normal text + **2 px green bar at top edge** of button
- Toggle OFF: transparent background + dim gray text (no bar)

## Produces
None

## Consumes
None

## Relevant Files
Modify: `src/ui/ControlStrip.cpp` — in `setupStatusBar()`: add
  `truePeakLimitingButton_.setComponentID("truePeakStatus");`
Modify: `src/ui/LookAndFeel.cpp` — in `drawButtonBackground()`: add a branch for
  `button.getComponentID() == "truePeakStatus"` that draws a 2.5 px green rect at the top
  when `button.getToggleState()` is true, and nothing otherwise.
Read: `src/ui/Colours.h` — `buttonOnText` (bright green), `accentBlue` — the indicator bar
  colour should be approximately `juce::Colour(0xff44CC44)`.
Read: `src/ui/ControlStrip.h` — `truePeakLimitingButton_` declaration.

## Acceptance Criteria
- [ ] Run: `cd /workspace/M-LIM && cmake --build build -j$(nproc) 2>&1 | tail -3` → Expected: `Built target MLIM_Standalone`
- [ ] Run: `cd /workspace/M-LIM/build && ctest --output-on-failure` → Expected: all tests pass
- [ ] Visual: Screenshot with True Peak ON → button has green top bar, no background fill
- [ ] Visual: Screenshot with True Peak OFF → button shows gray text, no bar, no fill

## Tests
None

## Technical Details
In `LookAndFeel.cpp::drawButtonBackground`, add before the default path:
```cpp
if (button.getComponentID() == "truePeakStatus")
{
    if (button.getToggleState())
    {
        g.setColour (juce::Colour (0xff44CC44));
        g.fillRect (bounds.getX(), bounds.getY(), bounds.getWidth(), 2.5f);
    }
    return;  // no background fill for this button type
}
```

In `ControlStrip.cpp::setupStatusBar()`:
```cpp
truePeakLimitingButton_.setComponentID ("truePeakStatus");
```

Keep `textColourOnId` = `MLIMColours::buttonOnText` (green text) or reset to `textPrimary`
depending on whether the bar alone provides sufficient visual distinction.

## Dependencies
None
