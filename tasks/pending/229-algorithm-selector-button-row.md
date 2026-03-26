# Task 229: Algorithm Selector — Replace ComboBox with 8-Button Row

## Description
The current AlgorithmSelector uses a juce::ComboBox dropdown. The target UI (FabFilter Pro-L 2) shows a compact row of 8 segmented buttons, one per algorithm (Transparent, Punchy, Dynamic, Aggressive, Allround, Bus, Safe, Modern). The selected algorithm button is highlighted in blue; all others are dark/dim. This is a critical visual parity gap.

Implementation plan:
- Keep the internal `juce::ComboBox comboBox` as a hidden child component (NOT visible) — it remains the APVTS attachment point via `getComboBox()`.
- Add 8 `juce::TextButton` members, one per algorithm name in `kAlgorithmNames[]` order.
- Lay them out as 2 rows of 4 buttons within the component bounds (row 1: Transparent/Punchy/Dynamic/Aggressive, row 2: Allround/Bus/Safe/Modern).
- When a button is clicked: call `comboBox.setSelectedId(index+1, juce::sendNotificationSync)` to propagate through APVTS.
- When `comboBox.onChange` fires: update all button states (selected = blue, rest = dark).
- `setAlgorithm(int index)` must still update both comboBox and button states.
- `getAlgorithm()` reads from comboBox as before.
- The component background: draw a subtle rounded rectangle behind each row of buttons.
- Selected button style: `buttonOnColourId` = MLIMColours::accentBlue, text white bold.
- Unselected button style: `buttonColourId` = MLIMColours::buttonBackground (0xff242424), text = MLIMColours::textSecondary.

Do NOT modify ControlStrip.cpp or ControlStrip.h — the algorithm selector's bounds are already set there (2 knob-widths). The 2-row-of-4 layout must fit within whatever bounds the ControlStrip provides.

## Produces
Implements: AlgorithmSelectorButtonRow

## Consumes
None

## Relevant Files
Read: `src/ui/AlgorithmSelector.h` — current interface to preserve (keep getComboBox(), setAlgorithm(), getAlgorithm(), onAlgorithmChanged)
Modify: `src/ui/AlgorithmSelector.h` — add 8 TextButton members, remove ComboBox from visible layout
Modify: `src/ui/AlgorithmSelector.cpp` — replace ComboBox-visible layout with 8-button row
Read: `src/ui/Colours.h` — color constants to use (accentBlue, buttonBackground, textSecondary, textPrimary, panelBorder)
Read: `src/dsp/LimiterAlgorithm.h` — get kAlgorithmNames[] and kNumAlgorithms (=8)
Read: `src/ui/ControlStrip.cpp` — understand how algorithmSelector_ is sized and attached

## Acceptance Criteria
- [ ] Run: `cd /workspace/M-LIM && export CCACHE_DIR=/build-cache && cmake -B build -DCMAKE_BUILD_TYPE=Release -DCMAKE_CXX_COMPILER_LAUNCHER=ccache -DCMAKE_C_COMPILER_LAUNCHER=ccache 2>&1 | tail -5` → Expected: exits 0, no errors
- [ ] Run: `cd /workspace/M-LIM && cmake --build build --config Release -j$(nproc) 2>&1 | tail -10` → Expected: exits 0, build succeeds
- [ ] Run: `grep -c "TextButton" /workspace/M-LIM/src/ui/AlgorithmSelector.h` → Expected: 8 or more (one per algorithm)
- [ ] Run: `grep -c "ComboBox" /workspace/M-LIM/src/ui/AlgorithmSelector.h` → Expected: at least 1 (the hidden APVTS ComboBox must remain)

## Tests
None

## Technical Details
- The 8 algorithm names in order: Transparent, Punchy, Dynamic, Aggressive, Allround, Bus, Safe, Modern (from kAlgorithmNames[] in LimiterAlgorithm.h)
- Each row of 4 buttons divides the component width into 4 equal parts
- Row height = component height / 2, with 2px gap between rows
- Button font: juce::Font(MLIMColours::kFontSizeSmall) — small to fit names
- After the build, take a screenshot with Xvfb and verify 8 buttons are visible

## Dependencies
None
