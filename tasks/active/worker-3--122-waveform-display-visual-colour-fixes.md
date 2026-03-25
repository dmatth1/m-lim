# Task 122: Waveform Display Visual Colour Fixes

## Description
Three related colour/visual issues in the waveform display. All reference the Pro-L 2 reference frames (`v1-0005.png`, `v1-0006.png`).

Note: The GR fill colour fix (drawGainReduction → near-black) is handled separately by task 115 (active).

**Issue 1 — Input/output waveform fills are nearly invisible (Colours.h):**
`MLIMColours::inputWaveform` is `0x70202840` (dark navy at ~44% alpha) and `outputWaveform` is `0x60182848`. Over the dark background both render essentially black — waveform fills are invisible when audio plays.

Correct values (pixel-sampled from reference):
- `inputWaveform`:  `0xA8607898` — medium blue-purple, ~66% alpha
- `outputWaveform`: `0x804060A0` — slightly deeper blue, ~50% alpha

**Issue 2 — Output envelope line is amber instead of white (Colours.h):**
`MLIMColours::outputEnvelope` is `0x80B89040` (amber). Reference shows a white/cream curved line (~`#E0E8FF` at ~80% alpha).

Correct value:
- `outputEnvelope`: `0xCCDDE8FF` — near-white with slight blue tint, ~80% alpha

**Issue 3 — Background gradient too dark, grid lines invisible (Colours.h + WaveformDisplay.cpp):**
`displayGradientTop: 0xff0D0D12` and `displayGradientBottom: 0xff1A1A2E` are near-black. Reference shows a slightly warmer dark blue-grey.

Correct values:
- `displayGradientTop`:    `0xff12121C`
- `displayGradientBottom`: `0xff1E2030`

Grid lines in `WaveformDisplay::drawBackground()` are drawn at `0xff2A2A2A` — nearly invisible over the dark background. Change to `0xff2E3040` for subtle but legible horizontal stripes.

## Produces
None

## Consumes
None

## Relevant Files
Modify: `M-LIM/src/ui/Colours.h` — update `inputWaveform`, `outputWaveform`, `outputEnvelope`, `displayGradientTop`, `displayGradientBottom`
Modify: `M-LIM/src/ui/WaveformDisplay.cpp` — change grid line colour in `drawBackground()`
Read: `/reference-docs/video-frames/v1-0005.png` — waveform fills and output envelope
Read: `/reference-docs/video-frames/v1-0006.png` — same

## Acceptance Criteria
- [ ] Run: `grep 'inputWaveform\|outputWaveform\|outputEnvelope' M-LIM/src/ui/Colours.h` → Expected: `inputWaveform` alpha byte > `0x90`, `outputEnvelope` RGB all > 0xC0 (light/white)
- [ ] Run: `grep '0xff2A2A2A' M-LIM/src/ui/WaveformDisplay.cpp` → Expected: no matches (replaced)
- [ ] Run: `cmake --build build -j$(nproc) 2>&1 | tail -3` → Expected: build succeeds, exit 0

## Tests
None

## Technical Details
All Colours.h changes are constant value replacements. In WaveformDisplay.cpp, one-line change:
- In `drawBackground()`: `g.setColour (juce::Colour (0xff2A2A2A))` → `g.setColour (juce::Colour (0xff2E3040))`

## Dependencies
None
