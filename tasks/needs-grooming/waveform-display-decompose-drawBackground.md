# Task: Decompose WaveformDisplay::drawBackground() into smaller functions

## Description
`WaveformDisplay::drawBackground()` in `src/ui/WaveformDisplay.cpp` is approximately 150 lines performing 6+ distinct rendering tasks (background gradient, idle fills, mid-zone brightness, center tent, grid lines, dB labels). This makes the function hard to read, tune, and maintain.

Extract into focused helper methods.

## Produces
None

## Consumes
None

## Relevant Files
Modify: `M-LIM/src/ui/WaveformDisplay.cpp` — decompose drawBackground()
Modify: `M-LIM/src/ui/WaveformDisplay.h` — declare new private helper methods

## Acceptance Criteria
- [ ] Run: `cmake --build build --target MLIM_Standalone_Standalone -j$(nproc)` → Expected: builds successfully
- [ ] Run: `grep -c "void WaveformDisplay::draw" M-LIM/src/ui/WaveformDisplay.cpp` → Expected: at least 3 more draw* methods than before

## Tests
None (visual refactor, no behavior change — verify with screenshot)

## Technical Details
Extract these helpers from drawBackground():
1. `drawIdleFill(Graphics&, Rectangle<float>)` — idle zone gradient fills
2. `drawGridLines(Graphics&, Rectangle<float>)` — horizontal grid lines at dB intervals
3. `drawGridLabels(Graphics&, Rectangle<float>)` — dB scale text labels

Keep the zone brightness logic (mid-zone, center tent) inline or in a `drawZoneBrightness()` helper.

## Dependencies
None
