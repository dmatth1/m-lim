# Task 285: TopBar — Preset Label Background Contrast Too Low

## Description
The preset label in the TopBar is nearly invisible because its background colour
`MLIMColours::topBarPresetBackground = 0xff232323` is almost identical to the top bar's
gradient background (`0xff252525` at top → `0xff1F1F1F` at bottom — only 2 brightness units
apart at the gradient top).

In the M-LIM standalone screenshot, the preset name area appears as a solid dark expanse
with no visible demarcation from the surrounding toolbar. In Pro-L 2, the preset name has
a clearly different (slightly lighter) label area with a visible border, making the centred
preset name stand out as an interactive element.

### Required change in `Colours.h`:

```cpp
// BEFORE
const juce::Colour topBarPresetBackground   { 0xff232323 };

// AFTER
const juce::Colour topBarPresetBackground   { 0xff2D2D2D };  // noticeably lighter than bar bg (0xff252525)
```

This raises the brightness from 35 (0x23) to 45 (0x2D) — enough contrast to be clearly
visible while remaining dark and unobtrusive.

Additionally, in `TopBar.cpp` the preset label should use a subtle rounded border to make
the label area feel like a clickable preset button. Add in `TopBar::TopBar()`, after the
existing `presetLabel_` setup lines:

```cpp
presetLabel_.setColour (juce::Label::outlineColourId, MLIMColours::panelBorder);
```

### Visual target:
- Preset name (e.g. "Default") is clearly readable in the centre of the top bar
- Subtle border outlines the preset name area as a discrete element
- Background contrast: noticeable but not jarring against the dark bar

## Produces
None

## Consumes
None

## Relevant Files
Modify: `M-LIM/src/ui/Colours.h` — change `topBarPresetBackground` from `0xff232323` to `0xff2D2D2D`
Modify: `M-LIM/src/ui/TopBar.cpp` — add `outlineColourId` to `presetLabel_` setup
Read: `M-LIM/src/ui/TopBar.h` — TopBar layout and presetLabel_ declaration

## Acceptance Criteria
- [ ] Run: `cmake --build build --target MLIM_Standalone -j$(nproc) 2>&1 | tail -3` → Expected: build succeeds, exit 0
- [ ] Run: `cd build && ctest --output-on-failure 2>&1 | tail -3` → Expected: all tests pass
- [ ] Visual check: preset name (e.g. "Default") is clearly visible in the centre of the top bar with a perceptible background contrast against the bar gradient

## Tests
None

## Technical Details
- Only two lines change: one constant in Colours.h, one colour assignment in TopBar.cpp
- `panelBorder = 0xff333333` — used as the outline colour, dark gray that's slightly visible
- After the change, the preset background is 0xff2D2D2D; the bar gradient top is 0xff252525; contrast ratio is ~1.07:1 (just enough to demarcate the area)

## Dependencies
None
