# Task: LUFS Panel Background and Histogram Gradient Brightness Increase

## Description

Color sampling of the right panel region (right crop: 180x500+720+0) shows the LUFS panel
areas are darker than the reference equivalent:

| Area                    | M-LIM current | Reference | Gap          |
|-------------------------|--------------|-----------|--------------|
| Histogram (upper 55%)   | #342D2E      | #3C323D   | +8R, +5G, +15B |
| Readout rows (lower 45%)| #2F2B2B      | #413F4C   | +18R, +20G, +33B |

Three `Colours.h` constants need brightening:

### Change 1: `loudnessPanelBackground`
Background for the readout rows area (Momentary/Short-Term/Integrated/Range/True Peak text rows + large readout + buttons).

```cpp
// BEFORE:
const juce::Colour loudnessPanelBackground { 0xff2B2729 };  // rgb(43,39,41)
// AFTER:
const juce::Colour loudnessPanelBackground { 0xff373438 };  // rgb(55,52,56) — +12R, +13G, +15B
```

Moving halfway toward the reference value of #413F4C (rgb 65,63,76).

### Change 2: `loudnessHistogramTop`
Top of the histogram gradient (upper edge of LUFS panel).

```cpp
// BEFORE:
const juce::Colour loudnessHistogramTop    { 0xff2E2A2C };  // rgb(46,42,44)
// AFTER:
const juce::Colour loudnessHistogramTop    { 0xff3A3035 };  // rgb(58,48,53) — +12R, +6G, +9B
```

### Change 3: `loudnessHistogramBottom`
Bottom of the histogram gradient.

```cpp
// BEFORE:
const juce::Colour loudnessHistogramBottom { 0xff3A3133 };  // rgb(58,49,51)
// AFTER:
const juce::Colour loudnessHistogramBottom { 0xff423C3F };  // rgb(66,60,63) — +8R, +11G, +12B
```

**Expected improvement:** right region RMSE ~0.3–0.6pp.

## Produces
None

## Consumes
None

## Relevant Files
Modify: `src/ui/Colours.h` — three loudness panel colour constants
Read: `src/ui/LoudnessPanel.cpp` — to understand where constants are used

## Acceptance Criteria
- [ ] Run: build + screenshot + right panel RMSE → Expected: ≤ 22.1% (improvement from 22.36%)
- [ ] Run: full image RMSE → Expected: ≤ 20.74% (no regression from wave 18 baseline)
- [ ] Run: visual check → Expected: LUFS panel appears slightly lighter/warmer, still dark; no harsh change

## Tests
None

## Technical Details

In `src/ui/Colours.h`, locate and update the three constants:

```cpp
const juce::Colour loudnessPanelBackground { 0xff2B2729 };
const juce::Colour loudnessHistogramTop    { 0xff2E2A2C };
const juce::Colour loudnessHistogramBottom { 0xff3A3133 };
```

Change to:

```cpp
const juce::Colour loudnessPanelBackground { 0xff373438 };
const juce::Colour loudnessHistogramTop    { 0xff3A3035 };
const juce::Colour loudnessHistogramBottom { 0xff423C3F };
```

Build command:
```bash
export CCACHE_DIR=/build-cache
cmake --build /workspace/M-LIM/build --target MLIM_Standalone -j$(nproc)
```

Measure:
```bash
convert /reference-docs/reference-screenshots/prol2-main-ui.jpg \
    -crop 1712x1073+97+32 +repage -resize 900x500! /tmp/ref.png

pkill -f "Standalone/M-LIM" 2>/dev/null; sleep 1
DISPLAY=:99 /workspace/M-LIM/build/MLIM_artefacts/Release/Standalone/M-LIM &>/dev/null &
sleep 8; DISPLAY=:99 scrot /tmp/raw.png; pkill -f "Standalone/M-LIM"
convert /tmp/raw.png -crop 908x500+509+325 +repage -resize 900x500! /tmp/mlim.png

# Right panel RMSE
convert /tmp/mlim.png -crop 180x500+720+0 +repage /tmp/right-mlim.png
convert /tmp/ref.png  -crop 180x500+720+0 +repage /tmp/right-ref.png
compare -metric RMSE /tmp/right-ref.png /tmp/right-mlim.png /dev/null 2>&1

# Full RMSE
compare -metric RMSE /tmp/ref.png /tmp/mlim.png /dev/null 2>&1
```

If right RMSE worsens, revert all three changes. If it improves, keep as-is.
If only partially effective, try `loudnessPanelBackground` at `0xff333031` (halfway).

## Dependencies
None
