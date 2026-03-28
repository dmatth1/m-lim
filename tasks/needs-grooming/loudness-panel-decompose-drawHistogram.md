# Task: Decompose LoudnessPanel::drawHistogram() into smaller functions

## Description
`LoudnessPanel::drawHistogram()` in `src/ui/LoudnessPanel.cpp` is approximately 156 lines combining layout calculation, grid drawing, idle fill, histogram bars, target indicator, dB scale labels, and LUFS header. Extract into focused helpers for maintainability.

## Produces
None

## Consumes
None

## Relevant Files
Modify: `M-LIM/src/ui/LoudnessPanel.cpp` — decompose drawHistogram()
Modify: `M-LIM/src/ui/LoudnessPanel.h` — declare new private helpers

## Acceptance Criteria
- [ ] Run: `cmake --build build --target MLIM_Standalone_Standalone -j$(nproc)` → Expected: builds successfully
- [ ] Run: `grep -c "void LoudnessPanel::draw" M-LIM/src/ui/LoudnessPanel.cpp` → Expected: at least 3 more draw* methods than before

## Tests
None (visual refactor, no behavior change)

## Technical Details
Extract:
1. `drawHistogramGrid(Graphics&, Rectangle<int>)` — grid structure
2. `drawHistogramBars(Graphics&, Rectangle<int>)` — the actual bar rendering
3. `drawHistogramLabels(Graphics&, Rectangle<int>)` — dB scale and LUFS header
4. `drawTargetIndicator(Graphics&, Rectangle<int>)` — target line/arrow

## Dependencies
None
