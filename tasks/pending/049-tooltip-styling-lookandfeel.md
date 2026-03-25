# Task 049: Tooltip and Value Popup Styling

## Description
Pro-L 2 shows styled value tooltips when hovering over or dragging knobs and sliders. The current LookAndFeel task (003) doesn't specify tooltip styling. Add dark-themed tooltip rendering matching the plugin's visual style.

## Produces
None

## Consumes
LookAndFeelDefinition
ColoursDefinition

## Relevant Files
Modify: `M-LIM/src/ui/LookAndFeel.h` — add tooltip drawing override declarations
Modify: `M-LIM/src/ui/LookAndFeel.cpp` — implement drawTooltip, set tooltip colors
Read: `M-LIM/src/ui/Colours.h` — color constants

## Acceptance Criteria
- [ ] Run: `cd M-LIM && cmake --build build --target MLIM_VST3 -j$(nproc) 2>&1 | tail -3` → Expected: builds successfully
- [ ] Run: `grep -i "tooltip\|Tooltip" M-LIM/src/ui/LookAndFeel.cpp` → Expected: at least 1 match

## Tests
None (visual styling)

## Technical Details
- Override LookAndFeel_V4::drawTooltip() for dark background tooltip
- Tooltip background: panelBorder or slightly lighter than main background (#2A2A2A)
- Tooltip text: textPrimary color (#E0E0E0)
- Tooltip border: subtle, 1px, panelBorder color
- Set tooltip-related ColourIds in LookAndFeel constructor:
  - TooltipWindow::backgroundColourId
  - TooltipWindow::textColourId
  - TooltipWindow::outlineColourId
- Consider adding juce::Slider::setPopupDisplayEnabled(true, ...) for knobs to show value during drag
- Font size: slightly smaller than main UI text

## Dependencies
Requires task 003
