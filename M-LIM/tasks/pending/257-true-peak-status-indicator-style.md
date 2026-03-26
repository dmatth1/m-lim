# Task 257: Status Bar — True Peak Limiting Active Indicator Style

## Description
The "True Peak Limiting" button in the status bar currently renders with a subtle blue background
fill (`accentBlue.withAlpha(0.15f)`) and bright green text when toggled on. The FabFilter Pro-L 2
reference shows this as a plain text label with a **thin green horizontal bar drawn above the text**
(approximately 2px tall, same width as the text area), with no background fill change. When
inactive it shows as dim gray text.

Current behaviour:
- Toggle ON: subtle blue/teal background + bright green text
- Toggle OFF: transparent background + gray text

Target behaviour (matching reference):
- Toggle ON: **transparent background** + normal text colour + a **2 px green bar drawn at the top
  edge** of the button bounds (spanning the full button width)
- Toggle OFF: transparent background + dim gray text (no indicator bar)

## Produces
None

## Consumes
None

## Relevant Files
Modify: `src/ui/ControlStrip.cpp` — change `truePeakLimitingButton_` setup in `setupStatusBar()`:
  - Set `buttonOnColourId` to `juce::Colours::transparentBlack` (remove background fill)
  - Set `textColourOnId` to `MLIMColours::textPrimary` (white, not green)
  - Override the button's `paintButton` or use a custom `Component::paint` approach: subclass or
    use a `CustomButton` helper that draws a green top border when toggle state is on.

Read: `src/ui/LookAndFeel.cpp` — `drawButtonBackground` is called for all TextButtons; the green
  indicator bar needs to be drawn AFTER the background (either in `drawButtonBackground` when
  the button component ID matches, or via a custom component).
Read: `src/ui/Colours.h` — `buttonOnText` (currently bright green) and `accentBlue`; the new
  indicator bar colour should be a bright green similar to `buttonOnText`.
Read: `src/ui/ControlStrip.h` — button declarations to understand if a custom subclass is feasible.

## Acceptance Criteria
- [ ] Build: `cd /workspace/M-LIM && cmake --build build -j$(nproc)` → Expected: exit 0, no errors
- [ ] Visual: Launch standalone on Xvfb, take screenshot → "True Peak Limiting" button has **no
  background fill** when active (toggle ON), and shows a **green horizontal bar** at the top edge
  of the button bounds instead of a coloured background
- [ ] Visual: When toggle is OFF, "True Peak Limiting" shows as dim gray text with no indicator bar
- [ ] RMSE: `compare -metric RMSE` of status bar region ≤ 17% (improvement from current 18.6%)

## Tests
None

## Technical Details
The simplest approach is to add a guard in `MLIMLookAndFeel::drawButtonBackground` that checks
`button.getComponentID() == "truePeakStatus"` and, when toggle state is on, skips the background
fill and instead draws a 2 px green rectangle along the top of `bounds`. Alternatively, set the
component ID in `setupStatusBar()` and handle it in the LookAndFeel.

Steps:
1. In `setupStatusBar()`: `truePeakLimitingButton_.setComponentID("truePeakStatus");`
2. In `LookAndFeel.cpp::drawButtonBackground`: add a branch for `button.getComponentID() == "truePeakStatus"`:
   - If `button.getToggleState()`: fill a `juce::Rectangle<float>(0, 0, bounds.getWidth(), 2.5f)`
     with `juce::Colour(0xff44CC44)` (bright green); draw no background
   - Otherwise: draw no background (completely transparent)
3. Keep `textColourOnId` = `buttonOnText` (green) for the text colour change, OR reset it to
   `textPrimary` if the indicator bar alone is sufficient indication.

## Dependencies
None
