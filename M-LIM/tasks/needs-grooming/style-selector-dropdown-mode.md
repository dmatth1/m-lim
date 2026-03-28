# Task: Add Dropdown/Compact Mode for Algorithm Selector

## Description
The Pro-L 2 reference shows the algorithm/style selector as a compact dropdown displaying the current style name (e.g., "Modern") with left/right arrows for cycling. In M-LIM, the selector is a 2x4 grid of buttons (TR, PU, DY, AG, AR, BU, SA, MO) which takes up significantly more horizontal space in the control strip.

While the 2x4 button grid is functionally equivalent, it creates a large visual discrepancy in the control strip region. Adding a compact dropdown mode (or replacing the grid entirely) would better match the reference layout. The dropdown should:
1. Show the current algorithm name (e.g., "Modern", "Transparent", "Punchy", etc.)
2. Have left/right arrow buttons (« ») to cycle through algorithms
3. Clicking the name opens a dropdown list of all 8 options

## Produces
None

## Consumes
None

## Relevant Files
Modify: `src/ui/AlgorithmSelector.cpp` — add dropdown rendering mode
Modify: `src/ui/AlgorithmSelector.h` — add dropdown mode state
Read: `src/ui/ControlStrip.cpp` — layout context for the selector
Read: `src/dsp/LimiterAlgorithm.h` — algorithm enum and names

## Acceptance Criteria
- [ ] Run: `cmake --build build --target MLIM_Standalone -j$(nproc)` → Expected: builds successfully
- [ ] Run: visual comparison → Expected: style selector appears as compact dropdown with algorithm name, matching reference layout

## Tests
None

## Technical Details
- Algorithm names for dropdown: "Transparent", "Punchy", "Dynamic", "Aggressive", "Allround", "Bus", "Safe", "Modern"
- Compact mode dimensions: approximately same height as one knob, width ~120px
- Arrow buttons: small left/right arrows flanking the name text
- This is a significant visual change that will reduce control strip RMSE

## Dependencies
None
