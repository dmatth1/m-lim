# Task 048: Gain Reduction Peak Reset on Click

## Description
Pro-L 2 allows clicking the GR peak readout to reset the peak hold value. This interaction is missing from task 021 (GainReductionMeter). Add click-to-reset behavior on the peak display.

## Produces
None

## Consumes
GainReductionMeter

## Relevant Files
Modify: `M-LIM/src/ui/GainReductionMeter.h` — add mouseDown override, resetPeak method
Modify: `M-LIM/src/ui/GainReductionMeter.cpp` — implement click detection on peak label area, reset logic

## Acceptance Criteria
- [ ] Run: `cd M-LIM && cmake --build build --target MLIM_VST3 -j$(nproc) 2>&1 | tail -3` → Expected: builds successfully
- [ ] Run: `grep "resetPeak\|mouseDown" M-LIM/src/ui/GainReductionMeter.cpp` → Expected: at least 2 matches

## Tests
None (UI interaction)

## Technical Details
- Override mouseDown() in GainReductionMeter
- Detect if click is within the peak readout label area
- Reset peak hold value to 0 dB on click
- Cursor should change to pointing hand when hovering over the peak label (setMouseCursor)
- Also reset peak markers on the waveform display if applicable
- This is a small but important UX detail for professional use

## Dependencies
Requires task 021
