# Task 192: WaveformDisplay — Background Gradient Colours Don't Match Reference

## Description
The waveform display background gradient uses:
```cpp
const juce::Colour displayGradientTop    { 0xff12121C };  // near-black with slight blue tint
const juce::Colour displayGradientBottom { 0xff1E2030 };  // dark blue-gray
```

Comparing the running plugin screenshot against the reference:
- The current background appears very dark (near-black top → very dark navy bottom)
- The reference (`prol2-intro.jpg`, `v1-0005.png`, `v1-0040.png`) shows the waveform background as a more visible **medium-dark blue-gray**, approximately `#1A1E2E` to `#242840` range — clearly showing as a distinct dark blue, not near-black

The gradient should be adjusted so the waveform area has a more visible dark blue tone rather than appearing nearly identical to the application background (`0xff1a1a1a`).

Reference target colours (approximate from reference screenshots):
- Top: `0xff141828` (dark navy, slightly lighter than current)
- Bottom: `0xff202438` (medium dark blue, clearly distinguishable)

## Produces
None

## Consumes
None

## Relevant Files
Modify: `M-LIM/src/ui/Colours.h` — adjust `displayGradientTop` and `displayGradientBottom`
Read: `/reference-docs/video-frames/v1-0005.png` — reference for background colour at idle (no audio)
Read: `/reference-docs/reference-screenshots/prol2-intro.jpg` — full plugin reference

## Acceptance Criteria
- [ ] Run: `cmake --build build --config Release -j$(nproc) && echo OK` → Expected: `OK`
- [ ] Run: build plugin, screenshot — expected: waveform area shows a visibly dark blue background gradient, clearly distinct from the black application chrome
- [ ] Run: `cd build && ctest --output-on-failure` → Expected: all tests pass

## Tests
None (colour-only change)

## Technical Details
In `M-LIM/src/ui/Colours.h`, update:
```cpp
// BEFORE:
const juce::Colour displayGradientTop    { 0xff12121C };
const juce::Colour displayGradientBottom { 0xff1E2030 };

// AFTER (tuned to reference):
const juce::Colour displayGradientTop    { 0xff141828 };
const juce::Colour displayGradientBottom { 0xff1E2438 };
```

The exact values should be tuned by visually comparing a screenshot of the plugin against `v1-0005.png`. The key is that the bottom of the gradient should be noticeably more blue than the top, and the whole area should be darker than the control strip but lighter/bluer than pure black.

Also check `displayBackground = 0xff141414` (used for meter backgrounds) — this should remain as-is since it's a non-gradient element.

## Dependencies
None
