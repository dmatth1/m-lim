# Task 020: Level Meter Component

## Description
Create vertical stereo level meter with blue/yellow/red color zones, peak hold indicators, and smooth ballistics.

## Produces
Implements: `LevelMeterInterface`

## Consumes
ColoursDefinition

## Relevant Files
Create: `M-LIM/src/ui/LevelMeter.h` — class declaration
Create: `M-LIM/src/ui/LevelMeter.cpp` — implementation
Read: `M-LIM/src/ui/Colours.h` — color constants
Read: `SPEC.md` — LevelMeterInterface

## Acceptance Criteria
- [ ] Run: `cd M-LIM && cmake --build build --target MLIM_VST3 -j$(nproc) 2>&1 | tail -3` → Expected: builds without errors

## Tests
None (visual component)

## Technical Details
- Two vertical bars side by side (L/R channels)
- Color zones: blue (below -6dB), yellow (-6 to -1dB), red (above -1dB)
- Smooth ballistic falloff: ~20dB/sec fall rate
- Peak hold: thin white line at peak, holds for 2 seconds then falls
- dB scale markings on side: 0, -3, -6, -12, -18, -24, -48
- setLevel called from editor timer with smoothed values
- Background: displayBackground color
- Meter segments or gradient fill

## Dependencies
Requires task 003
