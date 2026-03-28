# Task 543: Waveform dB Scale Label and Grid Line Contrast Increase

## Description
The dB scale labels on the waveform display are at 0.35f alpha (barely visible) and grid lines at 0.25f alpha. The Pro-L 2 reference shows these at ~45-50% and ~35% respectively. Increase both.

## Produces
None

## Consumes
None

## Relevant Files
Modify: `M-LIM/src/ui/WaveformDisplay.cpp` — line 424: change label alpha from 0.35f to 0.48f; line 396: change grid line alpha from 0.25f to 0.33f

## Acceptance Criteria
- [ ] Run: `grep -n 'withAlpha.*0\.48' M-LIM/src/ui/WaveformDisplay.cpp` → Expected: shows dB label alpha 0.48f
- [ ] Run: `grep -n 'waveformGridLine.*withAlpha.*0\.33' M-LIM/src/ui/WaveformDisplay.cpp` → Expected: shows grid alpha 0.33f
- [ ] Run: `cd M-LIM && cmake --build build --config Release --target M-LIM_Standalone -j$(nproc)` → Expected: builds successfully

## Tests
None

## Dependencies
None
