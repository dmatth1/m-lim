# Task: Waveform dB Scale Label Contrast Increase

## Description
The dB scale labels on the waveform display are drawn at 35% alpha (`0.35f`), making them barely visible. The Pro-L 2 reference shows these labels at approximately 45-50% opacity — clearly readable but not dominant. Similarly, the waveform grid lines are at 25% alpha vs the reference's ~35%. Increasing both will improve visual parity across the waveform region (currently 16.27% RMSE) and especially the left-meter boundary.

## Produces
None

## Consumes
None

## Relevant Files
Modify: `src/ui/WaveformDisplay.cpp` — find the dB label alpha (search for `0.35f` near the scale label drawing code around line 410-420) and increase to `0.48f`
Modify: `src/ui/WaveformDisplay.cpp` — find the grid line alpha (search for `0.25f` near grid line drawing around line 395-405) and increase to `0.33f`

## Acceptance Criteria
- [ ] Run: Build standalone, launch headless, capture screenshot → Expected: dB labels visibly more readable on waveform left edge
- [ ] Run: Compare waveform crop (640x500+0+0) RMSE → Expected: Wave RMSE < 16%

## Tests
None

## Technical Details
The dB scale labels are drawn in WaveformDisplay.cpp around lines 408-426. Look for:
```cpp
g.setColour (MLIMColours::textSecondary.withAlpha (0.35f));
```
Change to `0.48f`.

For grid lines around lines 395-405:
```cpp
g.setColour (MLIMColours::waveformGridLine.withAlpha (0.25f));
```
Change to `0.33f`.

## Dependencies
None
