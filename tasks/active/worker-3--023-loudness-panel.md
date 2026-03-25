# Task 023: Loudness Panel Component

## Description
Create the loudness metering panel showing LUFS readings (momentary, short-term, integrated), loudness range, and true peak values with numeric displays and small bar meters.

## Produces
Implements: `LoudnessPanelInterface`

## Consumes
ColoursDefinition

## Relevant Files
Create: `M-LIM/src/ui/LoudnessPanel.h` — class declaration
Create: `M-LIM/src/ui/LoudnessPanel.cpp` — implementation
Read: `M-LIM/src/ui/Colours.h` — color constants
Read: `SPEC.md` — LoudnessPanelInterface

## Acceptance Criteria
- [ ] Run: `cd M-LIM && cmake --build build --target MLIM_VST3 -j$(nproc) 2>&1 | tail -3` → Expected: builds without errors

## Tests
None (visual component)

## Technical Details
- Layout: vertical stack of labeled readings
- Rows:
  1. "Momentary" — large numeric LUFS value + small horizontal bar
  2. "Short-Term" — large numeric LUFS value + small horizontal bar
  3. "Integrated" — large numeric LUFS value + small horizontal bar + reset button
  4. "Range" — LU value display
  5. "True Peak" — dBTP value display (L/R or max)
- Numeric values: formatted to 1 decimal place (e.g., "-14.2 LUFS")
- Bar meter colors: blue (safe), yellow (caution), red (exceeds target)
- Target reference line on bars (configurable, default -14 LUFS)
- Collapsible: can expand/collapse with animation
- Reset button clears integrated measurement
- Dark background matching main display area

## Dependencies
Requires task 003
