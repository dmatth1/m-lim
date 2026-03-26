# Task 244: Control Strip — True Peak Limiting Button and Status Bar Polish

## Description
The "True Peak Limiting" toggle button in the status bar appears as a large bright-blue
highlighted button (`buttonOnBackground` with bright green text), which is visually
jarring and inconsistent with Pro-L 2's status bar where this option is shown as a
subtle label-style toggle (highlighted text, not a filled button).

Additionally, the MIDI Learn button and other status bar elements use inconsistent
styling vs the reference.

**Fix**:
1. Restyle `truePeakLimitingButton_` in `ControlStrip.cpp` to use the same muted style
   as `oversamplingStatusLabel_`: dark/transparent background with coloured text that
   changes on toggle state (e.g., bright green when active, secondary gray when off),
   rather than a filled background.
2. The active state should show the text in `MLIMColours::buttonOnText` (bright green)
   against a transparent/dark background — not a filled `buttonOnBackground` green box.
3. The `midiLearnButton_` (left-most status item) should use the same style as
   `oversamplingStatusLabel_` — text-only, no button border.
4. Reduce `truePeakLimitingButton_` button colour to `juce::Colours::transparentBlack`
   (off state) and `MLIMColours::accentBlue.withAlpha(0.15f)` (on state — very subtle
   tint instead of solid background).

## Produces
None

## Consumes
None

## Relevant Files
Modify: `src/ui/ControlStrip.cpp` — update setupStatusBar() colours for truePeakLimitingButton_ and midiLearnButton_
Read:   `src/ui/Colours.h` — button colour constants to reference

## Acceptance Criteria
- [ ] Run: build, launch standalone → Expected: "True Peak Limiting" in status bar shows as a text label with no visible filled background; text turns bright when active
- [ ] Run: build → Expected: status bar looks like a strip of text labels, not a row of bordered buttons
- [ ] Run: `cd build && ctest --output-on-failure` → Expected: all tests pass

## Tests
None

## Technical Details
In `ControlStrip::setupStatusBar()`, replace the `truePeakLimitingButton_` colour setup:
```cpp
// OFF state: transparent background, secondary text
truePeakLimitingButton_.setColour(juce::TextButton::buttonColourId,
                                  juce::Colours::transparentBlack);
// ON state: very subtle blue tint, primary text
truePeakLimitingButton_.setColour(juce::TextButton::buttonOnColourId,
                                  MLIMColours::accentBlue.withAlpha(0.15f));
truePeakLimitingButton_.setColour(juce::TextButton::textColourOffId,
                                  MLIMColours::textSecondary);
truePeakLimitingButton_.setColour(juce::TextButton::textColourOnId,
                                  MLIMColours::buttonOnText);  // bright green
```

Similarly for `midiLearnButton_`:
```cpp
midiLearnButton_.setColour(juce::TextButton::buttonColourId, juce::Colours::transparentBlack);
midiLearnButton_.setColour(juce::TextButton::textColourOffId, MLIMColours::textSecondary);
```

## Dependencies
None
