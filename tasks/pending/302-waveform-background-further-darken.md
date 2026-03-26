# Task 302: Waveform Display — Further Darken Background Gradient

## Description
Task 295 darkened the waveform background gradient from the original values to:
- `displayGradientTop = #6E7A9A`
- `displayGradientBottom = #565E70`

The reference (Pro-L 2) still appears slightly darker than the current M-LIM state. The waveform
background at gaps between content in video frames (`v1-0009.png`) is approximately `#6A7488` (top)
to `#525A6A` (bottom) — suggesting a further small darkening is warranted.

**Fix**: Apply a small additional darkening step:
- `displayGradientTop`: `#6E7A9A` → `#72808A`  (reduce blue saturation, slightly darker)
- `displayGradientBottom`: `#565E70` → `#505870`  (slightly darker)

This will make the static RMSE (without audio) closer to the reference.

## Produces
None

## Consumes
None

## Relevant Files
Modify: `M-LIM/src/ui/Colours.h` — update `displayGradientTop` and `displayGradientBottom`

## Acceptance Criteria
- [ ] Build passes: `cmake --build /workspace/M-LIM/build --config Release -j$(nproc)` → exits 0
- [ ] Visual: Waveform background is visibly darker than before — closer to the reference's dark waveform aesthetic
- [ ] Visual: Grid lines and dB labels remain legible against the darker background
- [ ] RMSE check: `compare -metric RMSE /workspace/screenshots/audit-ref-flat.png <(convert screenshot -crop 620x378+80+30 png:-) ...` → Expected: improvement vs current 26.16% waveform center RMSE

## Tests
None

## Technical Details
In `Colours.h`, change:
```cpp
// Before (post-task-295):
const juce::Colour displayGradientTop   { 0xff6E7A9A };
const juce::Colour displayGradientBottom{ 0xff565E70 };

// After:
const juce::Colour displayGradientTop   { 0xff72808A };  // reduce blue saturation, slightly darker
const juce::Colour displayGradientBottom{ 0xff505870 };  // slightly darker bottom
```

Also check `inputWaveform = 0xCC6878A0` — with a darker background, the waveform fill (at 0xCC
alpha = 80%) will composite differently. If the resulting waveform fill color drifts too far from
the reference, the `inputWaveform` alpha may need minor adjustment (try 0xDD or 0xBB).

## Dependencies
None
