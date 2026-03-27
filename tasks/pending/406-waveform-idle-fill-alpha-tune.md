# Task 406: Waveform Idle Fill — Bottom Alpha Reduction for Neutral Tone

## Description

The existing lower idle fill (fillFrac=0.56, alpha 0→0.88) uses the inputWaveform color
(#6878A0 = srgb 104,120,160) which has significant blue saturation. At the bottom of the
waveform (y=390), M-LIM shows srgb(104,121,162) while the reference shows srgb(119,125,153).

The reference at y=390 has:
- +15 more Red
- +4 more Green
- -9 LESS Blue

The high-alpha idle fill at the bottom (88% alpha at y=390) creates a blue cast stronger than
the reference. Reducing the maximum alpha from 0.88 to 0.82 allows more of the base gradient
to show through, producing a slightly warmer and less blue-saturated bottom.

**Fix**: Reduce idle fill bottom alpha from 0.88 to 0.82.

## Produces
None

## Consumes
None

## Relevant Files
Modify: `src/ui/WaveformDisplay.cpp` — line ~301 where `withAlpha(0.88f)` is set

## Acceptance Criteria
- [ ] Run: `cmake --build build --target MLIM_Standalone_Standalone -j$(nproc)` → Expected: exit 0
- [ ] Run wave RMSE → Expected: ≤ 16.72%
- [ ] Run full RMSE → Expected: ≤ 19.46%
- [ ] Run: `ls screenshots/task-406-rmse-results.txt` → Expected: file exists

## Tests
None

## Technical Details

**Current code** (WaveformDisplay.cpp ~line 299-304):
```cpp
juce::ColourGradient fillGrad (
    MLIMColours::inputWaveform.withAlpha (0.0f),   0.0f, fillTop,
    MLIMColours::inputWaveform.withAlpha (0.88f),  0.0f, area.getBottom(),
    false);
```

**Change to**:
```cpp
juce::ColourGradient fillGrad (
    MLIMColours::inputWaveform.withAlpha (0.0f),   0.0f, fillTop,
    MLIMColours::inputWaveform.withAlpha (0.82f),  0.0f, area.getBottom(),
    false);
```

**Tuning**: Try 0.80, 0.82, 0.84 and pick the value that gives best wave RMSE while not
degrading the full RMSE.

## Dependencies
None
