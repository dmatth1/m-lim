# Task 109: Waveform Background Gradient Too Dark, Grid Lines Invisible

## Description
Two related visual issues in `WaveformDisplay`:

**Issue 1 — Background gradient is too dark:**
`displayGradientTop: 0xff0D0D12` (near-black) and `displayGradientBottom: 0xff1A1A2E` (very dark navy). The reference (`v1-0005.png`, `v1-0006.png`) shows the waveform background as a slightly warmer dark blue-grey when no signal is present. The current near-black makes the display look dead/off even at idle.

Correct values:
- `displayGradientTop`:    `0xff12121C` — very dark blue-grey (slight step up from pure black)
- `displayGradientBottom`: `0xff1E2030` — dark blue-navy

These are subtle changes but they ensure the waveform area looks like an active dark display rather than pure black.

**Issue 2 — Horizontal dB grid lines have almost zero contrast:**
Grid lines are drawn at `0xff2A2A2A` on a background of `0xff0D0D12`–`0xff1A1A2E`. The difference is only about 15–20 lightness units — the lines are essentially invisible.

Reference frames show subtle but **clearly legible** horizontal grid lines at each dB step. They appear as slightly lighter horizontal stripes across the dark background.

Fix in `WaveformDisplay::drawBackground()` — change the grid line colour from `0xff2A2A2A` to `0xff2E3040` (a blue-grey that provides ~25% more contrast against the dark background gradient while remaining subtle).

## Produces
None

## Consumes
None

## Relevant Files
Modify: `M-LIM/src/ui/Colours.h` — update `displayGradientTop`, `displayGradientBottom`
Modify: `M-LIM/src/ui/WaveformDisplay.cpp` — change hardcoded grid line colour `0xff2A2A2A` in `drawBackground()`
Read: `/reference-docs/video-frames/v1-0005.png` — grid lines clearly visible as horizontal stripes
Read: `/reference-docs/video-frames/v1-0006.png` — same

## Acceptance Criteria
- [ ] Run: `grep 'displayGradientTop\|displayGradientBottom' M-LIM/src/ui/Colours.h` → Expected: both values present and changed from original
- [ ] Run: `grep '0xff2A2A2A' M-LIM/src/ui/WaveformDisplay.cpp` → Expected: no matches (replaced with new value)
- [ ] Run: `cmake --build build -j$(nproc) 2>&1 | tail -3` → Expected: build succeeds

## Tests
None

## Technical Details
The grid line colour change is in `WaveformDisplay::drawBackground()` at the line:
```cpp
g.setColour (juce::Colour (0xff2A2A2A));
```
Change to:
```cpp
g.setColour (juce::Colour (0xff2E3040));
```

This single change is in WaveformDisplay.cpp; the gradient changes are in Colours.h.

## Dependencies
None
