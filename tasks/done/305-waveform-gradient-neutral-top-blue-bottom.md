# Task 305: Waveform Background Gradient — Neutral Top, Blue-Saturated Bottom

## Description
The waveform background gradient currently drifts too far from the reference in two ways:

1. **Top of waveform is too cool/blue** — M-LIM shows `#72808A` (114, 128, 138) at the top,
   but the reference (main UI screenshot, where waveform content is densest at the top)
   averages to approximately `#6B696F` (107, 105, 111) — a warmer, more neutral gray.
   M-LIM's top is 20+ units brighter and over-saturated in the blue channel.

2. **Center/bottom is too gray (under-saturated in blue)** — M-LIM shows `#626D7D` (98, 109, 125)
   at the center, but the reference shows `#646E92` (100, 110, 146) — significantly more blue
   (B channel 146 vs M-LIM's 125, a 21-unit gap).

**Measured reference values (prol2-main-ui.jpg crop):**
- Top-left waveform: `#6B696F` = (107, 105, 111)
- Center waveform: `#646E92` = (100, 110, 146)

**Fix in `Colours.h`:**
```cpp
// BEFORE:
const juce::Colour displayGradientTop   { 0xff72808A };
const juce::Colour displayGradientBottom{ 0xff505870 };

// AFTER:
const juce::Colour displayGradientTop   { 0xff686468 };  // neutral/warm gray, matches ref top
const juce::Colour displayGradientBottom{ 0xff506090 };  // more blue-saturated, matches ref center
```

The gradient creates a top that blends naturally with the warm waveform content and a bottom that
shows the characteristic blue cast visible in Pro-L 2's waveform background.

## Produces
None

## Consumes
None

## Relevant Files
Modify: `M-LIM/src/ui/Colours.h` — change `displayGradientTop` and `displayGradientBottom`
Read: `/reference-docs/video-frames/v1-0005.png` — clear waveform background with minimal signal
Read: `/reference-docs/reference-screenshots/prol2-main-ui.jpg` — dense-signal reference

## Acceptance Criteria
- [ ] Run: `cmake --build /workspace/M-LIM/build --config Release -j$(nproc) --target MLIM_Standalone 2>&1 | tail -3` → Expected: exit 0
- [ ] Visual: Launch standalone, screenshot. Waveform background top should look more neutral/grayish (less blue) than previous; bottom should have a richer blue cast.
- [ ] RMSE check (waveform sub-region 600x400+150+50): Run `compare -metric RMSE <ref-crop> <mlim-crop> /dev/null 2>&1` → Expected: improvement from 23.03%

## Tests
None

## Technical Details
The gradient interpolation is vertical (top → bottom). After the change:
- Top: `#686468` = rgb(104, 100, 104) — slightly warm neutral
- Bottom: `#506090` = rgb(80, 96, 144) — more saturated blue

This better matches the reference pattern where:
- The top area is influenced by warm waveform content → appears neutral/warm
- The lower area shows the pure blue-gray background → appears saturated blue

**Do not** darken more than specified — tasks 295 and 302 showed that over-darkening the top
degrades the appearance when audio is present.

## Dependencies
None
