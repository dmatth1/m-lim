# Task 371: Fix Left Meter Regression — Brighten Loudness Panel Histogram Background

## Description

The left meter sub-region RMSE regressed from 20.85% (wave 14) to 28.11% (wave 15), a +7.26pp
regression. Root cause: task 368 darkened `displayGradientTop` from `0xff686468` (R=104,G=100,B=104)
to `0xff3A3540` (R=58,G=53,B=64), which improved the waveform region. But task 367 made the
LoudnessPanel histogram area reuse this same gradient, so the loudness panel histogram also became
much darker.

**Pixel analysis (wave 15 state):**
- Left meter region average — Reference: R=91, G=92, B=112
- Left meter region average — M-LIM: R=58, G=57, B=70
- Gap: ~33 units too dark across all channels

The left meter harness crop (x=640-720 in the 900x500 image) captures:
- ~14px: right edge of waveform display
- 12px: GainReductionMeter (dark, ~#231417)
- ~54px: left portion of LoudnessPanel histogram area

The loudness panel histogram (54px wide in this crop) is the dominant controllable element. It
currently uses `displayGradientTop → displayGradientBottom` which gives a dark gradient. We need
a custom, brighter gradient dedicated to the loudness panel histogram to restore left meter RMSE.

**Fix:** In `LoudnessPanel::paint()`, replace the `displayGradientTop/Bottom` references in the
histogram background with dedicated, brighter colour constants. Add two new constants to Colours.h:
`loudnessHistogramTop` and `loudnessHistogramBottom` targeting the reference histogram appearance.

**Target values (based on pixel analysis):**
- `loudnessHistogramTop`:    `0xff60687A`  → (96, 104, 122)  — muted cool blue-gray
- `loudnessHistogramBottom`: `0xff7890B8`  → (120, 144, 184) — richer blue

These would make the loudness panel histogram average approximately R=108, G=124, B=153, bringing
the left meter crop average to ~R=88, G=92, B=112 (matching reference R=91, G=92, B=112).

**Fine-tuning guidance:** If left RMSE doesn't improve adequately, try brightening both by 10-15
units. If the histogram looks too bright visually, reduce by 8-10 units. The key constraint is
that the histogram content (scale labels, bars, target line) must remain readable.

## Produces
None

## Consumes
None

## Relevant Files
Modify: `src/ui/Colours.h` — add `loudnessHistogramTop` and `loudnessHistogramBottom` constants
Modify: `src/ui/LoudnessPanel.cpp` — replace `displayGradientTop/Bottom` in `paint()` histogram gradient
Read: `src/ui/LoudnessPanel.h` — find `kReadoutAreaH` and panel layout constants

## Acceptance Criteria
- [ ] Run: full RMSE measurement (correct methodology) → Expected: Full ≤ 21.30% (no regression)
- [ ] Run: left meter RMSE → Expected: left meter ≤ 22.00% (improvement from 28.11%)
- [ ] Run: right panel RMSE → Expected: right panel ≤ 25.00% (no significant regression from 23.94%)
- [ ] Run: Visual inspection → Expected: loudness panel histogram shows visible blue-gray gradient
      matching the general brightness of the reference's meter area at idle

## Tests
None

## Technical Details

**In `src/ui/Colours.h`, add after `loudnessPanelBackground`:**
```cpp
// Loudness panel histogram area gradient (brighter than waveform gradient to match reference)
const juce::Colour loudnessHistogramTop    { 0xff60687A };  // muted cool blue-gray
const juce::Colour loudnessHistogramBottom { 0xff7890B8 };  // richer blue
```

**In `src/ui/LoudnessPanel.cpp`, in `paint()`, change the histogram gradient from:**
```cpp
juce::ColourGradient grad = juce::ColourGradient::vertical (
    MLIMColours::displayGradientTop,
    histBoundsF.getY(),
    MLIMColours::displayGradientBottom,
    histBoundsF.getBottom());
```
**To:**
```cpp
juce::ColourGradient grad = juce::ColourGradient::vertical (
    MLIMColours::loudnessHistogramTop,
    histBoundsF.getY(),
    MLIMColours::loudnessHistogramBottom,
    histBoundsF.getBottom());
```

**Measurement methodology (use correct RMSE crop):**
```bash
# Build standalone
cd /workspace/M-LIM && export CCACHE_DIR=/build-cache
cmake --build build --target MLIM_Standalone -j$(nproc)

# Launch on virtual display
pkill -f "Standalone/M-LIM" 2>/dev/null; sleep 1
DISPLAY=:99 /workspace/M-LIM/build/MLIM_artefacts/Release/Standalone/M-LIM &
sleep 8
DISPLAY=:99 scrot /tmp/task-raw.png
pkill -f "Standalone/M-LIM"

# Correct crops
convert /reference-docs/reference-screenshots/prol2-main-ui.jpg \
    -crop 1712x1073+97+32 +repage -resize 900x500! /tmp/ref.png
convert /tmp/task-raw.png -crop 908x500+509+325 +repage -resize 900x500! /tmp/mlim.png

# Full RMSE
compare -metric RMSE /tmp/ref.png /tmp/mlim.png /dev/null 2>&1

# Left meter RMSE (80px, x=640-720)
convert /tmp/mlim.png -crop 80x500+640+0 +repage /tmp/mlim-left.png
convert /tmp/ref.png  -crop 80x500+640+0 +repage /tmp/ref-left.png
compare -metric RMSE /tmp/mlim-left.png /tmp/ref-left.png /dev/null 2>&1

# Right panel RMSE (180px, x=720-900)
convert /tmp/mlim.png -crop 180x500+720+0 +repage /tmp/mlim-right.png
convert /tmp/ref.png  -crop 180x500+720+0 +repage /tmp/ref-right.png
compare -metric RMSE /tmp/mlim-right.png /tmp/ref-right.png /dev/null 2>&1
```

**Wave 15 baseline (task-370):**
- Full: 21.30%
- Wave: 19.44%
- Left meters: 28.11% ← regression to fix
- Right panel: 23.94%
- Control strip: 20.65%

## Dependencies
None
