# Task 334: Algorithm Selector — Implement 8-Button Grid Layout

## Description
Task 229 was completed but used a ‹ NAME › navigation widget instead of the specified
8-button grid layout. The reference Pro-L 2 shows 8 compact algorithm buttons in 2 rows
of 4 within the STYLE section.

Current implementation: `AlgorithmSelector.h/cpp` uses `prevButton_` (‹), `nameLabel_`,
and `nextButton_` (›). This visually differs from the reference which shows individual
buttons for all 8 algorithms simultaneously.

The STYLE section occupies 1/6 of the control strip width (approximately 125px after knob
row is reverted to kKnobRowH=56 in task 328). Layout: 2 rows × 4 buttons.

**Implementation:**
- Add 8 `juce::TextButton algoButtons_[8]` members
- In constructor: style each button, set onClick to select algorithm via comboBox
- In `updateButtonStates()`: set buttonOn for selected, buttonOff for others
- In `resized()`: arrange in 2 rows of 4 (each row ~= height/2, each button = width/4)
- In `paint()`: draw subtle background behind the button grid
- Keep existing `prevButton_`, `nextButton_`, `nameLabel_` members but make them invisible
  OR remove them (they're no longer needed for navigation)
- The hidden `comboBox` APVTS attachment point must remain unchanged

**Button styling:**
- Inactive: background `buttonBackground` (#242424), text `textSecondary`
- Active/selected: background `accentBlue` with alpha 0.8, text `textPrimary`

## Produces
None

## Consumes
None

## Relevant Files
Modify: `src/ui/AlgorithmSelector.h` — add `juce::TextButton algoButtons_[8]`, keep comboBox
Modify: `src/ui/AlgorithmSelector.cpp` — implement 2×4 grid layout, remove nav widget
Read: `src/ui/Colours.h` — accentBlue, buttonBackground, textSecondary, textPrimary
Read: `src/dsp/LimiterAlgorithm.h` — kAlgorithmNames[] and kNumAlgorithms

## Acceptance Criteria
- [ ] Run: `grep -c "algoButtons_\[" /workspace/M-LIM/src/ui/AlgorithmSelector.h` → Expected: `1` (array declaration)
- [ ] Run: `cd /workspace/M-LIM && cmake --build build -j$(nproc) 2>&1 | grep -c "error:"` → Expected: `0`
- [ ] Build and screenshot → Expected: STYLE section shows 2 rows of 4 compact algorithm buttons, selected one highlighted blue

## Tests
None

## Technical Details
In `AlgorithmSelector.h`, add:
```cpp
juce::TextButton algoButtons_[kNumAlgorithms];
```

In constructor:
```cpp
for (int i = 0; i < kNumAlgorithms; ++i)
{
    algoButtons_[i].setButtonText (kAlgorithmButtonLabels[i]);
    algoButtons_[i].setClickingTogglesState (false);
    algoButtons_[i].setColour (juce::TextButton::buttonColourId,  MLIMColours::buttonBackground);
    algoButtons_[i].setColour (juce::TextButton::buttonOnColourId, MLIMColours::accentBlue.withAlpha (0.8f));
    algoButtons_[i].setColour (juce::TextButton::textColourOffId, MLIMColours::textSecondary);
    algoButtons_[i].setColour (juce::TextButton::textColourOnId,  MLIMColours::textPrimary);
    algoButtons_[i].onClick = [this, i]
    {
        comboBox.setSelectedId (i + 1, juce::sendNotificationSync);
    };
    addAndMakeVisible (algoButtons_[i]);
}
prevButton_.setVisible (false);
nextButton_.setVisible (false);
nameLabel_.setVisible (false);
```

In `updateButtonStates()`:
```cpp
const int selected = juce::jlimit (0, kNumAlgorithms - 1, comboBox.getSelectedId() - 1);
for (int i = 0; i < kNumAlgorithms; ++i)
    algoButtons_[i].setToggleState (i == selected, juce::dontSendNotification);
nameLabel_.setText (kAlgorithmButtonLabels[selected], juce::dontSendNotification);
```

In `resized()`:
```cpp
auto b = getLocalBounds();
const int rowH = b.getHeight() / 2;
const int colW = b.getWidth() / 4;
for (int i = 0; i < kNumAlgorithms; ++i)
{
    int row = i / 4;
    int col = i % 4;
    algoButtons_[i].setBounds (col * colW, row * rowH, colW, rowH);
}
```

## Dependencies
Requires task 328 (revert knob height so STYLE section has correct proportions)
