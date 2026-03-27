# Task 374: Right Panel — Warm Dark Histogram Background

## Description

The loudness panel histogram area uses a cool blue-tinted gradient
(`loudnessHistogramTop=0xff3A3540`, `loudnessHistogramBottom=0xff506090`) that was
originally copied from the waveform display gradient. The right panel in the reference
shows warm dark brownish tones, not cool blue-gray.

**Pixel analysis (right panel, x=720-900 in 900px crop):**

| Zone | Reference (R,G,B) | M-LIM (R,G,B) | Gap |
|------|-------------------|---------------|-----|
| Top (y=0–100)   | 53, 45, 50 — warm dark | 41, 39, 43 — cool dark | Blue excess +7 |
| Mid (y=100–300) | 73, 65, 58 — warm brown | 55, 55, 68 — cool blue  | Blue excess +17 |
| Low (y=300–400) | 47, 43, 51 — dark purple | 64, 63, 74 — cool blue  | Too bright & blue |

The reference right panel mid-zone is distinctly warm (B < G < R) while M-LIM is cool (R < G < B).

**Fix:** Change `loudnessHistogramTop` and `loudnessHistogramBottom` in `Colours.h` to warm
dark tones:
- `loudnessHistogramTop`:    `0xff3A3540` → `0xff2E2A2C`  (warm dark, R=46, G=42, B=44)
- `loudnessHistogramBottom`: `0xff506090` → `0xff3A3133`  (warm brownish mid, R=58, G=49, B=51)

Do NOT change `displayGradientTop`/`displayGradientBottom` (waveform display) — decoupled since task 371.

**Expected improvement:** ~1.5–2.5 pp reduction in right panel RMSE (23.57% → ~21–22%).

**Note:** Task 375 also modifies `Colours.h` (different constants: controlStripTop/Bottom).
These can run in parallel; workers must edit only their specified constants.

## Produces
None

## Consumes
None

## Relevant Files
Modify: `src/ui/Colours.h` — change `loudnessHistogramTop` and `loudnessHistogramBottom`
Read: `src/ui/LoudnessPanel.cpp` — how histogram gradient is used (lines ~216–222)
Skip: `src/ui/WaveformDisplay.cpp` — uses `displayGradientTop/Bottom`, not histogram constants

## Acceptance Criteria
- [ ] Run: build + screenshot + right panel RMSE → Expected: < 23.57% (improvement from wave 16)
- [ ] Run: full image RMSE → Expected: ≤ 21.22% (no regression)
- [ ] Run: visual check → Expected: histogram background visually warmer (brownish), not blue-gray

## Tests
None

## Technical Details

Change in `Colours.h` only:
```cpp
// Before:
const juce::Colour loudnessHistogramTop    { 0xff3A3540 };
const juce::Colour loudnessHistogramBottom { 0xff506090 };

// After:
const juce::Colour loudnessHistogramTop    { 0xff2E2A2C };  // warm dark
const juce::Colour loudnessHistogramBottom { 0xff3A3133 };  // warm brownish mid-tone
```

If these values worsen RMSE, try alternatives in order:
1. `top=0xff302C2E`, `bottom=0xff3C3234`
2. `top=0xff2B2729`, `bottom=0xff352F32`
3. `top=0xff332E31`, `bottom=0xff403538`

Build and measure:
```bash
export CCACHE_DIR=/build-cache
cmake --build /workspace/M-LIM/build --target MLIM_Standalone_Standalone -j$(nproc)

convert /reference-docs/reference-screenshots/prol2-main-ui.jpg \
    -crop 1712x1073+97+32 +repage -resize 900x500! /tmp/ref.png

pkill -f "Standalone/M-LIM" 2>/dev/null; sleep 1
DISPLAY=:99 /workspace/M-LIM/build/MLIM_artefacts/Release/Standalone/M-LIM &>/dev/null &
sleep 8
DISPLAY=:99 scrot /tmp/raw.png
pkill -f "Standalone/M-LIM"

convert /tmp/raw.png -crop 908x500+509+325 +repage -resize 900x500! /tmp/mlim.png

# Right panel RMSE
convert /tmp/mlim.png -crop 180x500+720+0 +repage /tmp/right-mlim.png
convert /tmp/ref.png  -crop 180x500+720+0 +repage /tmp/right-ref.png
compare -metric RMSE /tmp/right-ref.png /tmp/right-mlim.png /dev/null 2>&1

# Full RMSE
compare -metric RMSE /tmp/ref.png /tmp/mlim.png /dev/null 2>&1
```

## Dependencies
None
