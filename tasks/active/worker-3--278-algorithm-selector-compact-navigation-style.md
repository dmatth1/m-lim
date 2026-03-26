# Task 278: Algorithm Selector — Compact "‹ Name ›" Navigation Style

## Description
Pro-L 2's STYLE selector (visible in prol2-features.jpg, v1-0016, v1-0018, v1-0020) uses
a **compact single-row navigation widget**: `«` left arrow, current algorithm name centered,
`»` right arrow. It does NOT show 8 buttons simultaneously.

Current M-LIM `AlgorithmSelector` renders a **2-row × 4-column grid of 8 buttons**
(Trans / Punch / Dyn / Aggr / Allmd / Bus / Safe / Mod). This:
- Takes disproportionate horizontal space
- Does not match Pro-L 2's compact appearance
- Makes the control strip look different from the reference

Required changes:
1. **Remove the 8 `TextButton` members** from `AlgorithmSelector` (h + cpp).
2. **Add 2 `TextButton` members**: `prevButton_` ("‹") and `nextButton_` ("›").
3. **Add a `juce::Label` member**: `nameLabel_` displaying the current algorithm name.
4. **`resized()`**: 3-zone layout — `prevButton_` (20px) | `nameLabel_` (remainder) | `nextButton_` (20px).
5. **`paint()`**: Subtle dark rounded background + thin border (same as existing style).
6. **Click handlers**: prev/next cycle through algorithms 0–7, wrapping around, updating the
   hidden `comboBox` (which drives the APVTS attachment).
7. **`updateButtonStates()`**: Update `nameLabel_` text to `kAlgorithmNames[selectedIndex]`.
8. The hidden `comboBox` must remain as the APVTS attachment point.

The algorithm name should be displayed in ALL CAPS using the existing full algorithm name
(e.g., "TRANSPARENT", "PUNCHY", "DYNAMIC", etc. — from `kAlgorithmNames[]` in LimiterAlgorithm.h).

## Produces
None

## Consumes
None

## Relevant Files
Modify: `src/ui/AlgorithmSelector.h` — replace 8 button members with prevButton_, nextButton_, nameLabel_
Modify: `src/ui/AlgorithmSelector.cpp` — full rewrite of constructor, resized(), paint(), updateButtonStates()
Read: `src/dsp/LimiterAlgorithm.h` — kAlgorithmNames[] for display text
Read: `src/ui/Colours.h` — colour constants
Read: `src/ui/ControlStrip.cpp` — how AlgorithmSelector is used (line ~170, resized())
Skip: `src/dsp/` — not relevant except LimiterAlgorithm.h

## Acceptance Criteria
- [ ] Run: `cmake --build build -j$(nproc) 2>&1 | tail -3` → Expected: build succeeds, exit 0
- [ ] Run: `cd build && ctest --output-on-failure 2>&1 | tail -3` → Expected: all tests pass
- [ ] Run: `source Scripts/ui-test-helper.sh && start_app && screenshot screenshots/task-272-after.png && stop_app` → Expected: screenshot taken
- [ ] Visual check: STYLE section shows a single row with `‹` button, algorithm name text, `›` button — no 8-button grid visible
- [ ] Visual check: Clicking `›` cycles to next algorithm; clicking `‹` cycles to previous; wraps at ends
- [ ] Run: `compare_to_reference /reference-docs/reference-screenshots/prol2-main-ui.jpg screenshots/task-272-after.png 0.15` → Expected: RMSE reported (record result)

## Tests
None

## Technical Details
```cpp
// AlgorithmSelector.h — replace algoButtons_ array with:
juce::TextButton prevButton_;
juce::TextButton nextButton_;
juce::Label      nameLabel_;

// kAlgorithmButtonLabels — short display names in AlgorithmSelector.cpp:
// Possible short names: "Trans", "Punch", "Dyn", "Aggr", "Allrnd", "Bus", "Safe", "Mod"
// OR use ALL CAPS of kAlgorithmNames: "TRANSPARENT", "PUNCHY", etc.
// Prefer shorter versions to fit in the compact widget.
```

Layout in resized():
```cpp
auto b = getLocalBounds();
prevButton_.setBounds (b.removeFromLeft (20));
nextButton_.setBounds (b.removeFromRight (20));
nameLabel_.setBounds (b);
```

Style:
```cpp
nameLabel_.setFont (juce::Font (MLIMColours::kFontSizeMedium, juce::Font::bold));
nameLabel_.setColour (juce::Label::textColourId, MLIMColours::textPrimary);
nameLabel_.setJustificationType (juce::Justification::centred);
prevButton_.setButtonText (juce::CharPointer_UTF8 ("\xe2\x80\xb9"));  // ‹
nextButton_.setButtonText (juce::CharPointer_UTF8 ("\xe2\x80\xba"));  // ›
// Or simply: "<" and ">"
```

updateButtonStates():
```cpp
void AlgorithmSelector::updateButtonStates()
{
    const int selected = comboBox.getSelectedId() - 1;
    nameLabel_.setText (kAlgorithmButtonLabels[juce::jlimit (0, kNumAlgorithms-1, selected)],
                        juce::dontSendNotification);
}
```

Prev/next click handlers:
```cpp
prevButton_.onClick = [this] {
    int cur = comboBox.getSelectedId() - 1;
    int next = (cur - 1 + kNumAlgorithms) % kNumAlgorithms;
    comboBox.setSelectedId (next + 1, juce::sendNotificationSync);
};
nextButton_.onClick = [this] {
    int cur = comboBox.getSelectedId() - 1;
    int next = (cur + 1) % kNumAlgorithms;
    comboBox.setSelectedId (next + 1, juce::sendNotificationSync);
};
```

## Dependencies
None
