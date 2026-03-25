# Task 168: WaveformDisplay — Output Envelope Line Should Be Amber/Warm Not Blue-White

## Description
The `outputEnvelope` colour constant in `Colours.h` is currently:
```cpp
const juce::Colour outputEnvelope { 0xCCDDE8FF };  // near-white with slight blue tint
```
This produces a cool blue-white line in the waveform display. The reference Pro-L 2 shows the output envelope as a **warm amber/cream/golden line**, clearly visible as a warmer tone compared to the waveform fill.

Reference frames showing the amber envelope line:
- `/reference-docs/video-frames/v1-0005.png` — output envelope line is clearly a warm cream/golden colour against the dark blue background
- `/reference-docs/video-frames/v1-0040.png` — close-up shows the same warm amber output envelope line

The correct colour should be a warm white-gold, similar to `0xCCE8C060` (alpha ~80%, warm amber-white). A good match for the reference would be:
```cpp
const juce::Colour outputEnvelope { 0xCCE8D090 };  // warm amber/cream, ~80% alpha
```
or approximately `0xCCF0D870` (bright golden-cream) depending on the exact reference shade.

The current `0xCCDDE8FF` has a strong blue component (0xFF = 255 blue vs. 0xDD red, 0xE8 green), making it appear distinctly cool/blue. The reference line is warm (higher red+green, lower blue).

## Produces
None

## Consumes
None

## Relevant Files
Modify: `M-LIM/src/ui/Colours.h` — change `outputEnvelope` constant
Read: `/reference-docs/video-frames/v1-0005.png` — visual reference for amber colour
Read: `M-LIM/src/ui/WaveformDisplay.cpp` — confirm where `outputEnvelope` is used (line ~405)

## Acceptance Criteria
- [ ] Run: `cmake --build build --config Release -j$(nproc) && echo OK` → Expected: `OK`
- [ ] Run: build plugin, play audio, screenshot waveform display — expected: output envelope line appears warm amber/golden/cream, clearly warmer than the surrounding waveform fill colours
- [ ] Run: `cd build && ctest --output-on-failure` → Expected: all tests pass

## Tests
None (colour-only change; no new test required)

## Technical Details
Change one line in `M-LIM/src/ui/Colours.h`:

```cpp
// BEFORE:
const juce::Colour outputEnvelope { 0xCCDDE8FF };  // near-white with slight blue tint, ~80% alpha

// AFTER:
const juce::Colour outputEnvelope { 0xCCE8C878 };  // warm amber/cream, ~80% alpha
```

The hex `0xCCE8C878`: alpha=CC (80%), R=E8 (232), G=C8 (200), B=78 (120) produces a warm amber line. Tune within ±20 on the RGB components to match the reference screenshot. The key requirement is R > G > B for a warm/amber tone, and alpha around 0xCC (80%).

## Dependencies
None
