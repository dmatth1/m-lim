# Task 295: Darken Waveform Display Background Gradient to Match Reference

## Description
`WaveformDisplay::drawBackground()` fills the waveform area with a gradient:
- Top: `MLIMColours::displayGradientTop = #8892AA`
- Bottom: `MLIMColours::displayGradientBottom = #606878`

Pixel sampling of the reference (`prol2-main-ui.jpg`) in the waveform background area (avoiding peaks):
- Reference background at y≈160-300 in 900x500 image: `#636F98` to `#727FAA`
- Reference background lower half: `#6E7591` to `#787D9A`

M-LIM's top gradient (`#8892AA`) is too LIGHT compared to the reference. The reference background is noticeably darker and more blue-purple. The overly light background is a significant contributor to the waveform area RMSE (currently 25.79%).

**Required change:** In `src/ui/Colours.h`, update the two gradient constants:
```cpp
// Current (too light):
const juce::Colour displayGradientTop   { 0xff8892AA };
const juce::Colour displayGradientBottom{ 0xff606878 };

// Updated (darker, closer to reference):
const juce::Colour displayGradientTop   { 0xff6E7A9A };
const juce::Colour displayGradientBottom{ 0xff565E70 };
```

The top value moves from `#8892AA` → `#6E7A9A` (about 26 units darker in R/G/B).
The bottom value moves from `#606878` → `#565E70` (about 10 units darker).

This is a purely cosmetic change in `Colours.h` only — no logic changes needed.

## Produces
None

## Consumes
None

## Relevant Files
Modify: `M-LIM/src/ui/Colours.h` — update `displayGradientTop` and `displayGradientBottom`

## Acceptance Criteria
- [ ] Run: `cmake --build /workspace/M-LIM/build --config Release -j$(nproc) --target MLIM_Standalone 2>&1 | tail -3` → Expected: exit 0
- [ ] Visual: waveform area background visually darker and more blue-purple than before, closer to reference screenshot.
- [ ] Run RMSE full image on 900x500 → Expected: total RMSE ≤ 0.25 (improvement from 0.2570)

## Tests
None

## Technical Details
Pixel sampling methodology:
```bash
convert /tmp/ref-900.png -crop 1x1+400+200 txt:- 2>/dev/null  # → #6D799E
convert /tmp/ref-900.png -crop 1x1+400+250 txt:- 2>/dev/null  # → #6E7591
```
The reference background (excluding waveform peaks) clusters around `#636F98`–`#727FAA`.

## Dependencies
None
