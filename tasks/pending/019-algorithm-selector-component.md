# Task 019: Algorithm Selector Component

## Description
Create the algorithm/style selector UI component — a styled dropdown button showing the current algorithm name with a popup menu for selection.

## Produces
Implements: `AlgorithmSelectorInterface`

## Consumes
ColoursDefinition

## Relevant Files
Create: `M-LIM/src/ui/AlgorithmSelector.h` — class declaration
Create: `M-LIM/src/ui/AlgorithmSelector.cpp` — implementation
Read: `M-LIM/src/ui/Colours.h` — color constants
Read: `SPEC.md` — AlgorithmSelectorInterface

## Acceptance Criteria
- [ ] Run: `cd M-LIM && cmake --build build --target MLIM_VST3 -j$(nproc) 2>&1 | tail -3` → Expected: builds without errors

## Tests
None (visual component)

## Technical Details
- Styled button that shows current algorithm name (e.g. "Modern", "Transparent")
- Click opens popup menu with all 8 algorithm names
- Visual: dark background, cyan/blue text, rounded corners
- Supports APVTS ComboBoxAttachment for the "algorithm" parameter
- onAlgorithmChanged callback fires when selection changes
- Algorithm names: Transparent, Punchy, Dynamic, Aggressive, Allround, Bus, Safe, Modern

## Dependencies
Requires task 003
