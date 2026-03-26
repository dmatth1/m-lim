# Task 296: Waveform Display — Darken Background Gradient to Match Reference

## Description
The current M-LIM waveform background gradient uses:
- `displayGradientTop = #8892AA`
- `displayGradientBottom = #606878`

When no audio is playing, this creates a bright blue-gray background visible across the entire
waveform area (~620px × 378px at 900x500 scale). The Pro-L 2 reference screenshot shows the waveform
area as DARKER overall because the audio waveform fill (dark blue) covers most of the background.
However, even the background gradient in the reference (visible in gaps between waveform content)
appears slightly darker than M-LIM's current gradient.

Looking at the video frames (`v1-0009.png`), the waveform background at the gaps between content
appears approximately `#6A7488` (top) to `#525A6A` (bottom) — about 10-12% darker than current.

**Fix**: Darken the waveform gradient:
- `displayGradientTop`: `#8892AA` → `#72808A`  (darker, less blue)
- `displayGradientBottom`: `#606878` → `#505870`  (darker)

This will reduce the brightness mismatch in the empty waveform state and make the static RMSE
(without audio) closer to the reference.

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
// Before:
const juce::Colour displayGradientTop   { 0xff8892AA };
const juce::Colour displayGradientBottom{ 0xff606878 };

// After:
const juce::Colour displayGradientTop   { 0xff72808A };  // darker — matches reference gap colors
const juce::Colour displayGradientBottom{ 0xff505870 };  // darker bottom
```

Also check `inputWaveform = 0xCC6878A0` — with a darker background, the waveform fill (at 0xCC
alpha = 80%) will composite differently. If the resulting waveform fill color drifts too far from
the reference, the `inputWaveform` alpha may need minor adjustment (try 0xDD or 0xBB).

## Dependencies
None
