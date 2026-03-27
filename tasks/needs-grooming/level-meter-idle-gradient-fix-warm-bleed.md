# Task: Level Meter Idle Gradient — Remove Warm-Amber Bleed, Use Cool Blue

## Description
The level meter idle gradient in `LevelMeter.cpp::drawChannel()` was boosted by tasks 415–417
to add warm orange/yellow colours at idle. This has caused a significant Left zone RMSE
regression: **30.25% vs 23.50% baseline** (task-354).

Root cause analysis:
- The `warmExtY` stop at 20% from top uses `grMeterLow.withAlpha(0.20f)` (warm yellow #E8C840).
- Because JUCE `ColourGradient` interpolates colour AND alpha between stops, the warm yellow
  bleeds down through the remaining 80% of the bar via gradient interpolation toward the bottom.
- At y=25% from top, the composed result is warm amber ~(82,63,33), but the reference at the
  same position shows cool blue-purple (105,101,141).
- The bottom stop `meterSafe.darker(0.3f).withAlpha(0.80f)` produces only ~(65,72,94) due to
  the warm zone intercepting the gradient — insufficient brightness and too warm.

Pixel evidence (input meter x=5, idle state):
```
y=100:  M-LIM (79,60,31) warm-amber  | Ref (51,27,34)  — ref dark reddish-brown
y=200:  M-LIM (92,74,42) warm-amber  | Ref (105,101,141) — ref COOL blue-purple
y=250:  M-LIM (95,81,54) warm-amber  | Ref (90,97,129)  — ref cool blue-purple
y=300:  M-LIM (93,85,67) warm-amber  | Ref (137,143,172) — ref bright cool blue-purple
```

The correct appearance: the meter should be dark at the top (unfilled track) transitioning
to bright cool blue at the bottom (simulating active safe-zone fill). The reference shows
this cool blue-purple pattern because Pro-L 2 was captured with audio (~-14 dBFS) which
fills the lower portion with `meterSafe`-coloured segments.

## Produces
None

## Consumes
None

## Relevant Files
Modify: `src/ui/LevelMeter.cpp` — `drawChannel()` idle gradient block (lines ~88–117)
Read:   `src/ui/Colours.h` — `meterSafe`, `barTrackBackground`, `grMeterLow`, `grMeterMid`

## Acceptance Criteria
- [ ] Run: build + screenshot + Left zone RMSE (`compare -metric RMSE` on crop 30x378+0+30) → Expected: Left zone RMSE ≤ 25.00% (improvement from 30.25%)
- [ ] Run: Full RMSE → Expected: Full ≤ 19.50% (no significant regression)
- [ ] Run: Right zone RMSE (crop 100x400+800+50) → Expected: Right zone RMSE ≤ 26.74% (no regression)

## Tests
None

## Technical Details
In `LevelMeter.cpp drawChannel()`, idle structural gradient block:

**Current (causes warm-amber bleed through lower 80% of bar):**
```cpp
juce::ColourGradient idleGrad (
    MLIMColours::meterDanger.withAlpha (0.10f),              0.0f, barTop2,
    MLIMColours::meterSafe.darker (0.3f).withAlpha (0.80f),  0.0f, barTop2 + barH2,
    false);
idleGrad.addColour ((dangerBot2 - barTop2) / barH2,
                    MLIMColours::grMeterMid.withAlpha (0.28f));     // orange — too high
idleGrad.addColour ((warnBot2   - barTop2) / barH2,
                    MLIMColours::meterWarning.withAlpha (0.25f));   // yellow — too high
idleGrad.addColour ((warmExtY   - barTop2) / barH2,
                    MLIMColours::grMeterLow.withAlpha (0.20f));     // warm-yellow — BLEEDS
```

**Target (warm zone small, abrupt cut to cool blue at ~20% boundary):**
```cpp
juce::ColourGradient idleGrad (
    MLIMColours::meterDanger.withAlpha (0.08f),           0.0f, barTop2,
    MLIMColours::meterSafe.withAlpha (0.85f),             0.0f, barTop2 + barH2,
    false);
// Keep small warm hints at top to preserve any slight warm reference at top 5%
idleGrad.addColour ((dangerBot2 - barTop2) / barH2,
                    MLIMColours::grMeterMid.withAlpha (0.10f));     // orange — reduced
idleGrad.addColour ((warnBot2   - barTop2) / barH2,
                    MLIMColours::meterWarning.withAlpha (0.08f));   // yellow — reduced
// Cut warm zone sharply: replace warm-yellow bleed with cool blue transition stop
idleGrad.addColour ((warmExtY   - barTop2) / barH2,
                    MLIMColours::meterSafe.withAlpha (0.10f));      // cool blue — no warm bleed
```

Key changes:
1. Bottom stop: `meterSafe.darker(0.3f).alpha(0.80)` → `meterSafe.alpha(0.85)` gives ~(94,106,139)
   vs current ~(65,72,94). Target reference is (105,101,141) at y=200 → much closer.
2. Warm zone alphas: 0.28/0.25/0.20 → 0.10/0.08/0.10(cool) — eliminates warm-amber bleed.
3. warmExtY stop: warm-yellow replaced with cool meterSafe so gradient blends cool.

Verify RMSE for Left zone (30x378+0+30) and Full image before and after:
```bash
DISPLAY=:99 /path/to/MLIM &; sleep 5
import -window root /tmp/task-raw.png
convert /tmp/task-raw.png -crop 908x500+509+325 +repage -resize 900x500! /tmp/task-mlim.png
convert /reference-docs/reference-screenshots/prol2-main-ui.jpg -crop 1712x1073+97+32 +repage -resize 900x500! /tmp/task-ref.png
# Left zone
convert /tmp/task-mlim.png -crop 30x378+0+30 +repage /tmp/z-left.png
convert /tmp/task-ref.png  -crop 30x378+0+30 +repage /tmp/r-left.png
compare -metric RMSE /tmp/r-left.png /tmp/z-left.png /dev/null 2>&1
# Full
compare -metric RMSE /tmp/task-ref.png /tmp/task-mlim.png /dev/null 2>&1
```

Adjust alpha values empirically if initial results diverge. The goal is cool blue-purple at
the bottom 50% of the idle bar that approximates the reference's active fill appearance.

## Dependencies
None
