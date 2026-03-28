# Task: Waveform Lower Zone Brightness — Reduce inputWaveform Fill to Close 12-Unit Gap

## Description

Pixel measurement (current build vs reference) shows a persistent brightness gap in the
lower waveform zone (y=300 in 900x500 crop = ~71.5% down the waveform display):

- M-LIM current: (120, 125, 156) at x=300, y=300
- Reference:     (132, 138, 166) at x=300, y=300  (from task-408 measurement)
- Gap:           R+12, G+13, B+10 (M-LIM too dark)

**Important**: The midzone (y=250) is currently near-perfect:
- M-LIM current: (122, 128, 154)
- Reference:     (121, 128, 155)
Any fix MUST NOT worsen the midzone match.

**Root cause**: The `inputWaveform` idle fill uses `withAlpha(0.82f)` at the bottom of the
gradient fill area (y=100% of display). The inputWaveform color (104,120,160) is DARKER
than the displayGradientBottom (158,158,196), so high alpha makes the zone too dark. The
lower idle fill (lowerFill) at alpha 0.35 provides insufficient compensation.

**Fix approach** — two-part change to `WaveformDisplay.cpp`:

1. Reduce `inputWaveform` fill bottom alpha from `0.82f` to `0.70f`:
   ```cpp
   // line ~303 in WaveformDisplay.cpp
   MLIMColours::inputWaveform.withAlpha (0.70f),  // reduced from 0.82 (task-406)
   ```

2. Increase `waveformIdleLowFill` alpha from `0.35f` to `0.50f` AND lighten the constant
   from `0xff9898A8` = (152,152,168) to `0xffA8A8B8` = (168,168,184) in Colours.h:
   ```cpp
   // Colours.h
   const juce::Colour waveformIdleLowFill { 0xffA8A8B8 };  // lightened from 0xff9898A8

   // WaveformDisplay.cpp ~line 393
   lFill.withAlpha (0.50f),  // increased from 0.35
   ```

**Expected effect at y=300 (71.5% down waveform)**:
- Reducing inputWaveform alpha: slight brightening (+1.5R)
- Increasing lowerFill to 0.50 alpha with lighter color: +5R, +5G, +3B contribution
- Combined net improvement: approximately R≈125, G≈130, B≈162 (from current 120,125,156)
- Remaining gap to reference (132,138,166): R+7, G+8, B+4 — significantly reduced

**Effect at y=250 (midzone = 57.9% down)**:
- Lower fill starts at lTop=62%, so y=57.9% is NOT affected by lowerFill change ✓
- inputWaveform alpha reduction: fill fraction at 57.9% = (57.9-44)/56 = 0.248
  - Old alpha: 0.82*0.248=0.203, New: 0.70*0.248=0.174 — very small change
  - Effect on midzone: ~+1 to +2 units — within acceptable noise
- **Midzone match should be preserved** (reference at (121,128,155), tolerance ±3)

**Acceptance criteria** — use RMSE methodology:
```bash
# Build and capture screenshot
convert mlim-scrot.png -crop 908x500+509+325 +repage -resize 900x500! /tmp/mlim.png
convert ref.jpg -crop 1712x1073+97+32 +repage -resize 900x500! /tmp/ref.png
# Measure wave region
compare -metric RMSE <(convert /tmp/mlim.png -crop 640x500+0+0 +repage png:-) \
                     <(convert /tmp/ref.png -crop 640x500+0+0 +repage png:-) /dev/null 2>&1
# Must not be worse than 16.06% (task-409 baseline)
# Check midzone pixel: convert /tmp/mlim.png -crop 1x1+300+250 +repage txt:-
# Must be within ±3 of (121,128,155)
```

## Produces
None

## Consumes
None

## Relevant Files
Modify: `src/ui/WaveformDisplay.cpp` — reduce inputWaveform fill withAlpha from 0.82 to 0.70
Modify: `src/ui/WaveformDisplay.cpp` — increase lowerFill withAlpha from 0.35 to 0.50
Modify: `src/ui/Colours.h` — lighten waveformIdleLowFill from 0xff9898A8 to 0xffA8A8B8
Read: `screenshots/task-408-rmse-results.txt` — reference pixel at y=300 = (132,138,166)
Read: `screenshots/task-404-rmse-results.txt` — reference pixel at y=250 = (121,128,155)

## Acceptance Criteria
- [ ] Run: `convert /tmp/mlim.png -crop 1x1+300+300 +repage txt:-` → Expected: value R≥124, G≥128 (closer to reference 132,138,166 than current 120,125,156)
- [ ] Run: `convert /tmp/mlim.png -crop 1x1+300+250 +repage txt:-` → Expected: R in range 120–126, G in range 125–131 (preserve midzone ±3 of reference 121,128,155)
- [ ] Run: wave region RMSE compare → Expected: ≤ 16.06% (no regression from task-409 baseline)

## Tests
None

## Technical Details
- `waveformIdleLowFill` change is in Colours.h (single line)
- The two WaveformDisplay.cpp changes are in drawBackground()
- The inputWaveform fill is the LARGE gradient fill at lines ~295-305 (withAlpha 0.82 → 0.70)
- The lowerFill is the SMALL gradient fill at lines ~383-393 (withAlpha 0.35 → 0.50)
- These are the ONLY changes needed — do not modify mid-zone fills or displayGradientBottom

## Dependencies
None
