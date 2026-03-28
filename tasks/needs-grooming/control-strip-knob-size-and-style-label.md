# Task: Control Strip Style Selector Visual Match

## Description
The control strip shows an 8-button grid (TR/PU/DY/AG/AR/BU/SA/MO) in two rows for the algorithm selector. The Pro-L 2 reference shows a single rounded "Modern" button for the style selector, which is much cleaner. The knobs in the control strip also appear slightly smaller than the reference. The 2x4 button grid is a significant visual difference contributing to Control Strip RMSE (19.28%).

The algorithm selector should use a single compact display showing the current style name (e.g., "Transparent") as a rounded button/label, with click-to-cycle or dropdown behavior, matching the reference's clean single-button look.

## Produces
None

## Consumes
None

## Relevant Files
Read: `src/ui/AlgorithmSelector.h` — current 8-button implementation
Modify: `src/ui/AlgorithmSelector.cpp` — change to single-button display mode
Read: `src/ui/ControlStrip.cpp` — layout of the algorithm selector within the control strip
Read: `src/ui/Colours.h` — button colours

## Acceptance Criteria
- [ ] Run: Build standalone, launch headless, capture screenshot → Expected: algorithm selector shows single rounded button with current style name
- [ ] Run: Compare control strip crop (900x90+0+410) RMSE → Expected: Control RMSE < 18%

## Tests
None

## Technical Details
The AlgorithmSelector currently creates 8 TextButton instances in a 2x4 grid. Change to:
1. Show a single rounded rectangle button displaying the current algorithm name (full name, not abbreviation)
2. Click cycles through algorithms (or opens dropdown)
3. Button should use `algoButtonSelected` background with `textPrimary` text
4. Approximate size: ~120x28 pixels, rounded corners ~4px
5. Place to the right of the "STYLE" label in the same position

This is one of the biggest structural differences in the control strip vs reference.

## Dependencies
None
