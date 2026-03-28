# Task 544: Algorithm Selector Single-Button Display Mode

## Description
The control strip shows an 8-button grid (TR/PU/DY/AG/AR/BU/SA/MO) in two rows. The Pro-L 2 reference shows a single rounded "Modern" button for the style selector — much cleaner. This is one of the biggest structural differences in the control strip contributing to Control Strip RMSE (19.28%).

Change the AlgorithmSelector from 8-button grid to a single compact display showing the current style name as a rounded button/label, with click-to-cycle or dropdown behavior.

## Produces
None

## Consumes
None

## Relevant Files
Modify: `M-LIM/src/ui/AlgorithmSelector.cpp` — replace 8-button grid with single-button display
Read: `M-LIM/src/ui/AlgorithmSelector.h` — current 8-button implementation
Read: `M-LIM/src/ui/ControlStrip.cpp` — layout context
Read: `M-LIM/src/ui/Colours.h` — button colours

## Acceptance Criteria
- [ ] Run: `cd M-LIM && cmake --build build --config Release --target M-LIM_Standalone -j$(nproc)` → Expected: builds successfully
- [ ] Run: Build and launch headlessly, capture screenshot → Expected: algorithm selector shows single rounded button with current style name (e.g. "Transparent")

## Tests
None

## Technical Details
1. Show a single rounded rectangle button displaying the current algorithm name (full name, not abbreviation)
2. Click cycles through algorithms (or opens dropdown)
3. Use `algoButtonSelected` background with `textPrimary` text
4. Approximate size: ~120x28 pixels, rounded corners ~4px
5. Place to the right of the "STYLE" label in the same position

## Dependencies
None
