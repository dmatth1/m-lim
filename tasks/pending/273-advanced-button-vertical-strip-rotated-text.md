# Task 273: ADVANCED Button — Narrow Vertical Strip with Rotated Text

## Description
In Pro-L 2 (clearly visible in prol2-features.jpg and v1-0003), the "ADVANCED" control is
a **narrow vertical strip on the far right of the control strip**, displaying the text
"ADVANCED" rotated 90° (reading bottom-to-top). It is approximately 18–20 px wide and
spans the full knob row height.

Current M-LIM has:
- `kAdvancedBtnW = 72` — a wide 72px rectangular button showing "ADVANCED >>" text
- Occupies significant horizontal space in the control strip

Required changes:
1. **ControlStrip.h / ControlStrip.cpp**: Change `kAdvancedBtnW` from 72 → 18.
2. **`advancedButton_` paint**: Override the button appearance by setting it as
   transparent and drawing the rotated "ADVANCED" text manually in `ControlStrip::paint()`,
   OR use a custom `LookAndFeel` for just this button.
3. The button should use the same click behavior (toggle advanced mode), just with the
   new visual style.
4. Remove the `advancedButton_.setButtonText (...)` calls (text is drawn manually).
5. The strip color should be `MLIMColours::algoButtonInactive` when collapsed,
   `MLIMColours::accentBlue.withAlpha(0.7f)` when expanded.

Alternative approach (simpler): In `ControlStrip::paint()`, after the existing drawing,
draw the rotated ADVANCED text manually at the button's bounds. Set the button's own colors
to transparent (no visible button text or background). The click area remains functional.

## Produces
None

## Consumes
None

## Relevant Files
Modify: `src/ui/ControlStrip.cpp` — change kAdvancedBtnW, update paint() to draw rotated text, adjust resized()
Modify: `src/ui/ControlStrip.h` — if kAdvancedBtnW is defined here
Read: `src/ui/Colours.h` — color constants
Read: `reference-docs/reference-screenshots/prol2-features.jpg` — shows ADVANCED strip clearly

## Acceptance Criteria
- [ ] Run: `cmake --build build -j$(nproc) 2>&1 | tail -3` → Expected: build succeeds, exit 0
- [ ] Run: `cd build && ctest --output-on-failure 2>&1 | tail -3` → Expected: all tests pass
- [ ] Run: `source Scripts/ui-test-helper.sh && start_app && screenshot screenshots/task-273-after.png && stop_app` → Expected: screenshot taken
- [ ] Visual check: "ADVANCED" appears as narrow rotated-text vertical strip on the right side of the control strip (not a wide horizontal button)
- [ ] Visual check: Clicking the strip still toggles the advanced state (button text changes color)
- [ ] Run: `compare_to_reference /reference-docs/reference-screenshots/prol2-main-ui.jpg screenshots/task-273-after.png 0.15` → Expected: RMSE reported (record result)

## Tests
None

## Technical Details
In `ControlStrip.cpp`:
```cpp
static constexpr int kAdvancedBtnW = 18;  // was 72
```

Configure the button to be invisible (transparent bg, no text drawn by default):
```cpp
advancedButton_.setButtonText ("");  // no text
advancedButton_.setColour (juce::TextButton::buttonColourId,   juce::Colours::transparentBlack);
advancedButton_.setColour (juce::TextButton::buttonOnColourId, juce::Colours::transparentBlack);
```

In `ControlStrip::paint()`, after other drawing, render the rotated "ADVANCED" text:
```cpp
{
    auto advB = advancedButton_.getBounds();
    juce::Graphics::ScopedSaveState ss (g);
    g.addTransform (juce::AffineTransform::rotation (
        -juce::MathConstants<float>::halfPi,
        advB.getCentreX(), advB.getCentreY()));

    // Rotated rect: same centre, width/height swapped
    auto rotatedR = juce::Rectangle<float> (
        advB.getCentreX() - advB.getHeight() / 2.0f,
        advB.getCentreY() - advB.getWidth() / 2.0f,
        (float) advB.getHeight(),
        (float) advB.getWidth());

    // Background fill
    const bool expanded = isAdvancedExpanded_;
    g.setColour (expanded ? MLIMColours::accentBlue.withAlpha (0.7f)
                          : MLIMColours::algoButtonInactive);
    g.fillRoundedRectangle (rotatedR, 3.0f);

    // Text
    g.setFont (juce::Font (MLIMColours::kFontSizeSmall, juce::Font::bold));
    g.setColour (expanded ? MLIMColours::textPrimary : MLIMColours::textSecondary);
    g.drawText ("ADVANCED", rotatedR, juce::Justification::centred, false);
}
```

Note: `isAdvancedExpanded_` is already a member of ControlStrip, used in the existing
`advancedButton_.onClick` lambda.

## Dependencies
None
