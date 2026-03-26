# Task 205: ADVANCED Panel — Vertical Rotated Side Tab on Control Strip Right Edge

## Description
The current implementation shows "ADVANCED >>" as a horizontal TextButton inside the knob row of the control strip. The FabFilter Pro-L 2 reference shows "ADVANCED" as a **vertical tab on the right edge** of the control strip, with the text rotated 90° clockwise.

### Reference Behaviour (video frames v1-0003, v1-0004):
- "ADVANCED" label rotated 90° counterclockwise on a narrow vertical tab
- Tab is flush against the right edge of the control strip knob area
- Clicking the tab expands/collapses an advanced panel (or simply toggles a label state)
- The tab appears as a dark rectangle with subtle border and secondary-colour text

### Required Change:
1. In `ControlStrip.h`/`.cpp`: remove `advancedButton_` TextButton from the knob row layout
2. Add a custom `AdvancedTab` component (or draw directly in `ControlStrip::paint`) that renders text rotated 90° counterclockwise ("ADVANCED") on a narrow strip on the right edge of the knob row
3. The tab should be clickable (toggle `isAdvancedExpanded_`) and visually indicate state (slightly lighter bg when active)
4. The actual advanced content panel (if any) is out of scope — just match the visual appearance of the collapsed tab

## Produces
None

## Consumes
None

## Relevant Files
Modify: `src/ui/ControlStrip.h` — remove advancedButton_ TextButton, add advancedTabRect_ juce::Rectangle<int> or small component
Modify: `src/ui/ControlStrip.cpp` — draw rotated "ADVANCED" text in paint(), handle click in mouseDown(), remove old advancedButton_ layout
Read: `src/ui/Colours.h` — colour constants (textSecondary, panelBorder, buttonBackground)
Read: `/reference-docs/video-frames/v1-0003.png` — visual reference

## Acceptance Criteria
- [ ] Run: `cmake --build build --target MLIM_Standalone -j$(nproc) 2>&1 | tail -3` → Expected: `[100%] Built target MLIM_Standalone`
- [ ] Run: visual inspection — right edge of control strip knob area shows narrow vertical "ADVANCED" tab with rotated text (no horizontal button)

## Tests
None

## Technical Details
- Rotated text: use `juce::Graphics::addTransform(juce::AffineTransform::rotation(-juce::MathConstants<float>::halfPi, cx, cy))` then `drawText`
- Tab width: ~20px, full height of knob row
- Reserve this width from the right of the knob row in `resized()` before laying out knobs
- In `paint()`: draw the tab rect with `MLIMColours::buttonBackground`, draw rotated "ADVANCED" text with `MLIMColours::textSecondary`
- In `mouseDown()`: detect click on tab rect → toggle state → repaint

## Dependencies
Requires task 204 (algorithm selector now single button, freeing up horizontal knob row space)
