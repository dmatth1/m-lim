# Task 241: Waveform Ceiling Line — Solid Line Instead of Dashed

## Description
The Pro-L 2 reference shows the output ceiling line as a **solid, thin horizontal line**
drawn across the waveform. M-LIM currently draws a dashed pattern (alternating 6 px dash
+ 4 px gap) using a loop in `WaveformDisplay::drawCeilingLine()`.

The ceiling line color was already updated to warm red (`0xCCDD4444`) in `Colours.h`.
This task changes the dash pattern to a simple solid line.

Additionally, the ceiling line thickness should be 1.5 px to be clearly visible on the
dark waveform background.

**Fix**: In `WaveformDisplay::drawCeilingLine()`, replace the dash-loop with a single
`g.drawLine()` call.

## Produces
None

## Consumes
None

## Relevant Files
Modify: `src/ui/WaveformDisplay.cpp` — replace dash-loop in drawCeilingLine() with single drawLine
Read:   `src/ui/WaveformDisplay.h` — method signature
Read:   `src/ui/Colours.h` — waveformCeilingLine colour is already set correctly

## Acceptance Criteria
- [ ] Run: build, launch standalone → Expected: ceiling line at top of waveform is a solid warm-red horizontal line (no dashes)
- [ ] Run: `cd build && ctest --output-on-failure` → Expected: all tests pass

## Tests
None

## Technical Details
In `WaveformDisplay::drawCeilingLine()`, replace the entire `float x = lineX0; bool drawing = true;` while-loop block with:
```cpp
g.drawLine(lineX0, y, lineX1, y, 1.5f);
```
The ceiling label on the scale side remains unchanged.

## Dependencies
None
