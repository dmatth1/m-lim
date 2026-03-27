# Task: Revert LoudnessPanel Histogram Waveform Gradient Background

## Description

Task 367 changed the LoudnessPanel histogram area background to use the waveform display gradient
(`displayGradientTop` → `displayGradientBottom`). Pixel analysis of the wave 15 state shows this
change hurts RMSE more than it helps.

**Pixel analysis (y=200, from auditor scan):**

Reference layout at x=660–800 (the LoudnessPanel region in the 900×500 crop):

| Region | Reference colors | M-LIM after task 367 | Diff | M-LIM flat #2B2729 | Diff |
|--------|-----------------|----------------------|------|--------------------|------|
| x=660–710, y=50–150 | ~#211F22 (32,31,34) dark | #3F4156 (63,65,86) | ~38 | #2B2729 (43,39,41) | ~10 |
| x=660–710, y=200–350 | ~#667299 (meter fill) | #4B5780 (75,87,128) | ~27 | #2B2729 (43,39,41) | ~82 |
| x=720–800, y=50–150 | ~#1D1B20 (29,27,32) dark | #4B5780 (75,87,128) | ~64 | #2B2729 (43,39,41) | ~11 |
| x=720–800, y=200–350 | ~#201E23 (32,30,35) dark | #4B5780 (75,87,128) | ~64 | #2B2729 (43,39,41) | ~10 |

**Total pixel-diff estimate:**
- Task 367 gradient improves x=660–710 y=200–350: ~55 diff/px saved × 50px × 150px = 412,500 units
- Task 367 gradient worsens x=660–710 y=50–150: ~28 more diff/px × 50px × 100px = 140,000 units
- Task 367 gradient worsens x=720–800 y=50–400: ~54 more diff/px × 80px × 350px = 1,512,000 units

**Net: task 367 gradient costs ~1,239,500 more RMSE units than flat background.**

The reference x=660–800 is mostly NEAR-BLACK at all y positions (the reference layout has a dark
separator + dark histogram background there). The waveform gradient creates blue tones (#4B5780) that
are far from the reference's near-black (#1D1B20 to #261C1D).

## Produces
None

## Consumes
None

## Relevant Files
Modify: `src/ui/LoudnessPanel.cpp` — revert `paint()` histogram area fill to use `loudnessPanelBackground`
Read: `src/ui/Colours.h` — `loudnessPanelBackground` = #2B2729, `displayGradientTop/Bottom` (do not change)
Skip: `src/ui/LoudnessPanel.h` — no changes needed

## Acceptance Criteria
- [ ] Run: Build + screenshot → compare right-half RMSE → Expected: improved vs current 23.94% (task-370 coords) or 28.5% (correct-baseline coords)
- [ ] Run: `compare -metric RMSE /tmp/ref.png /tmp/mlim.png /dev/null` on the 180×500 crop at x=720 → Expected: lower than current

## Tests
None

## Technical Details

**In `src/ui/LoudnessPanel.cpp` `paint()`, find (~line 215):**

```cpp
// Histogram area: waveform gradient background (matches reference level meter visual)
juce::ColourGradient grad = juce::ColourGradient::vertical (
    MLIMColours::displayGradientTop,
    histBoundsF.getY(),
    MLIMColours::displayGradientBottom,
    histBoundsF.getBottom());
g.setGradientFill (grad);
g.fillRoundedRectangle (getLocalBounds().toFloat(), 4.0f);
```

**Replace with:**

```cpp
// Histogram area: flat dark background (reference shows near-black in this region)
g.setColour (MLIMColours::loudnessPanelBackground);
g.fillRoundedRectangle (getLocalBounds().toFloat(), 4.0f);
```

This reverts the task 367 gradient change and goes back to the flat `loudnessPanelBackground` for the
histogram. The readout area flat fill below (line 225–226) remains unchanged.

**Measurement methodology (use CORRECT coordinates matching task-354 baseline):**

```bash
# Build
cd /workspace/M-LIM && export CCACHE_DIR=/build-cache && cmake --build build --target MLIM_Standalone -j$(nproc)

# Screenshot
DISPLAY=:99 /workspace/M-LIM/build/MLIM_artefacts/Release/Standalone/M-LIM &
sleep 8
DISPLAY=:99 scrot /tmp/task-wave16-raw.png
pkill -f "Standalone/M-LIM"

# Crop
convert /tmp/task-wave16-raw.png -crop 908x500+509+325 +repage -resize 900x500! /tmp/mlim-crop.png
convert /reference-docs/reference-screenshots/prol2-main-ui.jpg \
    -crop 1712x1073+97+32 +repage -resize 900x500! /tmp/ref-crop.png

# Full RMSE
compare -metric RMSE /tmp/ref-crop.png /tmp/mlim-crop.png /dev/null 2>&1

# Right panel (task-370 coords)
convert /tmp/mlim-crop.png -crop 180x500+720+0 +repage /tmp/mlim-right.png
convert /tmp/ref-crop.png  -crop 180x500+720+0 +repage /tmp/ref-right.png
compare -metric RMSE /tmp/ref-right.png /tmp/mlim-right.png /dev/null 2>&1
```

**If RMSE worsens:** The revert is net-negative — do NOT commit, instead try a much darker gradient:
```
displayHistGradientTop = 0xff1E1C21   (very dark, close to reference near-black)
displayHistGradientBottom = 0xff232028
```
Add these as new constants in Colours.h and use them instead of reverting entirely.

**Save results to:** `screenshots/task-NNN-rmse-results.txt`

## Dependencies
None
