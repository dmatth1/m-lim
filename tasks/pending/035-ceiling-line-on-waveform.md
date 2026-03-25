# Task 035: Output Ceiling Reference Line on Waveform Display

## Description
Pro-L 2 shows the output ceiling level as a horizontal reference line on the waveform display, making it visually clear where limiting starts. This is missing from the waveform display task (022). Add a ceiling indicator line that tracks the outputCeiling parameter.

## Produces
None

## Consumes
WaveformDisplayInterface
ParameterLayout

## Relevant Files
Modify: `M-LIM/src/ui/WaveformDisplay.h` — add setCeiling(float dB) method
Modify: `M-LIM/src/ui/WaveformDisplay.cpp` — draw horizontal ceiling line in paint()
Read: `M-LIM/src/ui/Colours.h` — color constants

## Acceptance Criteria
- [ ] Run: `cd M-LIM && cmake --build build --target MLIM_VST3 -j$(nproc) 2>&1 | tail -3` → Expected: builds successfully
- [ ] Run: `grep "ceiling\|Ceiling" M-LIM/src/ui/WaveformDisplay.cpp` → Expected: at least 2 matches

## Tests
None (visual component)

## Technical Details
- Horizontal line drawn at the dB position corresponding to the output ceiling value
- Color: subtle white or light gray, dashed or dotted line style
- Small label on the right showing the ceiling dB value (e.g., "-0.1 dB")
- Line should be drawn BEHIND the waveform fills but above the background grid
- Update via setCeiling(float dB) called from editor timer alongside other meter updates
- When ceiling is 0 dB, line should be at the very top of the display

## Dependencies
Requires task 022
