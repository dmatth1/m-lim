# Task: Right Panel — Warm Dark Histogram Background

## Description

The loudness panel histogram area uses a cool blue-tinted gradient
(`loudnessHistogramTop=0xff3A3540`, `loudnessHistogramBottom=0xff506090`) that was originally
copied from the waveform display gradient. However, the right panel in the reference shows
warm dark brownish tones throughout the histogram area, not cool blue-gray.

**Pixel analysis (right panel, x=720-900 in 900px crop):**

| Zone | Reference (R,G,B) | M-LIM (R,G,B) | Gap |
|------|-------------------|---------------|-----|
| Top (y=0–100)   | 53, 45, 50 — warm dark | 41, 39, 43 — cool dark | Blue excess +7 |
| Mid (y=100–300) | 73, 65, 58 — warm brown | 55, 55, 68 — cool blue  | Blue excess +17 |
| Low (y=300–400) | 47, 43, 51 — dark purple | 64, 63, 74 — cool blue  | Too bright & blue |

The reference right panel mid-zone is distinctly warm (B < G < R) while M-LIM is cool (R < G < B).
This causes large color-distance RMSE even though the brightness difference is modest.

**Fix:** Change `loudnessHistogramTop` and `loudnessHistogramBottom` in `Colours.h` to warm dark
tones that match the reference panel's brownish-warm appearance:

- `loudnessHistogramTop`:    `0xff3A3540` → `0xff2E2A2C`  (warm dark: R=46, G=42, B=44, close to loudnessPanelBackground 0xff2B2729)
- `loudnessHistogramBottom`: `0xff506090` → `0xff3A3133`  (warm mid: R=58, G=49, B=51, brownish)

The empty histogram background will then appear warm-dark like the reference rather than
blue-gray. This should move the right-panel region from cool to warm tone and reduce RMSE.

**Do NOT change** `displayGradientTop`/`displayGradientBottom` (waveform display). These are
now decoupled in `Colours.h` (task 371).

**Risk assessment:** Low. The loudness histogram gradient colors are independent from the
waveform gradient (decoupled in task 371). Only affects the LUFS panel histogram area.
The readout background uses separate `loudnessPanelBackground` and is unaffected.

**Expected improvement:** ~1.5–2.5 pp reduction in right panel RMSE (23.57% → ~21–22%).

## Produces
None

## Consumes
None

## Relevant Files
Modify: `M-LIM/src/ui/Colours.h` — change `loudnessHistogramTop` and `loudnessHistogramBottom`
Read: `M-LIM/src/ui/LoudnessPanel.cpp` — see how histogram gradient is used (lines 216–222)

## Acceptance Criteria
- [ ] Run: screenshot + RMSE check → Expected: right panel RMSE decreases from 23.57%
- [ ] Run: build and screenshot → Expected: histogram background visually warmer (brownish) not blue
- [ ] Run: full image RMSE → Expected: ≤ 21.22% (no regression)

## Tests
None

## Technical Details

Change in `Colours.h` only:
```cpp
// Before:
const juce::Colour loudnessHistogramTop    { 0xff3A3540 };  // matches displayGradientTop
const juce::Colour loudnessHistogramBottom { 0xff506090 };  // matches displayGradientBottom

// After:
const juce::Colour loudnessHistogramTop    { 0xff2E2A2C };  // warm dark, matches panel background warmth
const juce::Colour loudnessHistogramBottom { 0xff3A3133 };  // warm brownish mid-tone
```

If these exact values worsen RMSE, try the following alternatives in order:
1. `loudnessHistogramTop=0xff302C2E`, `loudnessHistogramBottom=0xff3C3234` (slightly lighter)
2. `loudnessHistogramTop=0xff2B2729`, `loudnessHistogramBottom=0xff352F32` (matching panel bg)
3. `loudnessHistogramTop=0xff332E31`, `loudnessHistogramBottom=0xff403538` (warmer)

Measure RMSE after each attempt using:
```bash
# Build
export CCACHE_DIR=/build-cache && cmake --build /workspace/M-LIM/build --target MLIM_Standalone -j$(nproc)

# Reference
convert /reference-docs/reference-screenshots/prol2-main-ui.jpg \
    -crop 1712x1073+97+32 +repage -resize 900x500! /tmp/ref.png

# Capture
pkill -f "M-LIM" 2>/dev/null; sleep 1
DISPLAY=:99 /workspace/M-LIM/build/MLIM_artefacts/Release/Standalone/M-LIM &>/dev/null &
sleep 8
DISPLAY=:99 scrot /tmp/raw.png
pkill -f "M-LIM"
convert /tmp/raw.png -crop 908x500+509+325 +repage -resize 900x500! /tmp/mlim.png

# Right panel RMSE
convert /tmp/mlim.png -crop 180x500+720+0 +repage /tmp/right-mlim.png
convert /tmp/ref.png  -crop 180x500+720+0 +repage /tmp/right-ref.png
compare -metric RMSE /tmp/right-ref.png /tmp/right-mlim.png /dev/null 2>&1
```

## Dependencies
None
