# Task 204: Algorithm Selector — Revert to Single-Button STYLE Section

## Description
The current AlgorithmSelector shows 8 separate buttons in a 2×4 grid (from task 229). The FabFilter Pro-L 2 reference clearly shows a **single "Modern" button** in the STYLE section of the control strip — clicking it opens a popup/dropdown to select among algorithms.

The 8-button grid takes up far too much horizontal space in the control strip and does not match the reference layout. The STYLE section in the reference is compact: a single button showing the current algorithm name.

### Reference Behaviour (from video frames v1-0002, v1-0003, v1-0004):
- Single pill-button labelled with the current algorithm name (e.g. "Modern")
- Left-click → opens popup menu listing all 8 algorithm names
- The STYLE section occupies ~1 knob slot width

### Required Change:
- In `AlgorithmSelector.cpp`/`.h`: hide the 8 individual buttons; show instead a single `juce::TextButton` whose label is the current algorithm name
- Clicking the button opens a `juce::PopupMenu` listing all 8 algorithm names
- Selecting from the popup updates the ComboBox (which is already APVTS-bound) and refreshes the button label
- The hidden ComboBox attachment remains so APVTS still works
- Style the button with `MLIMColours::buttonBackground` bg and `MLIMColours::textPrimary` text

## Produces
None

## Consumes
None

## Relevant Files
Modify: `src/ui/AlgorithmSelector.h` — replace 8 TextButton members with single `selectorButton_` TextButton
Modify: `src/ui/AlgorithmSelector.cpp` — implement popup-menu click, label update, remove 8-button paint/resized logic
Read: `src/ui/Colours.h` — button colour constants
Read: `src/dsp/LimiterAlgorithm.h` — kAlgorithmNames array

## Acceptance Criteria
- [ ] Run: `cmake --build build --target MLIM_Standalone -j$(nproc) 2>&1 | tail -3` → Expected: `[100%] Built target MLIM_Standalone`
- [ ] Run: visual inspection — STYLE section shows one compact button (not 8 buttons) occupying ~1 knob slot

## Tests
None

## Technical Details
- Keep `comboBox` (juce::ComboBox) as hidden child for APVTS attachment
- Button text: `comboBox.getText()` after setting item
- Popup: `juce::PopupMenu` with items `kAlgorithmNames[0..7]`, result → `comboBox.setSelectedId(result)`
- In `resized()`: single button fills the full bounds; no grid layout needed

## Dependencies
None
