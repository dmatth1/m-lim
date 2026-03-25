# Task 021: Gain Reduction Meter Component

## Description
Create the gain reduction meter that shows current GR with peak hold and numeric display, styled to match Pro-L 2's red GR indicators.

## Produces
Implements: `GainReductionMeter` (part of WaveformDisplayInterface area)

## Consumes
ColoursDefinition

## Relevant Files
Create: `M-LIM/src/ui/GainReductionMeter.h` — class declaration
Create: `M-LIM/src/ui/GainReductionMeter.cpp` — implementation
Read: `M-LIM/src/ui/Colours.h` — color constants

## Acceptance Criteria
- [ ] Run: `cd M-LIM && cmake --build build --target MLIM_VST3 -j$(nproc) 2>&1 | tail -3` → Expected: builds without errors

## Tests
None (visual component)

## Technical Details
- Vertical bar meter showing gain reduction amount (grows downward from 0dB)
- Color: gainReduction red
- Peak hold indicator with numeric dB readout
- Scale: 0 to -24dB (or configurable range)
- Smooth ballistics matching main waveform display
- Numeric display shows current GR and peak GR values
- setGainReduction(float dB) and setPeakGR(float dB) methods

## Dependencies
Requires task 003
