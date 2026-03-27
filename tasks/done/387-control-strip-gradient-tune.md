# Task: Control Strip — Brighten Inactive Algo Buttons and Knob Area Background

## Description

Color sampling of the control strip region shows two gaps:

| Area             | M-LIM current | Reference | Gap       |
|------------------|--------------|-----------|-----------|
| Algo buttons (left ~100px) | #262831 | #4E4E5D | +40R, +38G, +44B |
| Knob area (center ~200px)  | #504E5E | #565767 | +6R, +9G, +9B |

The algorithm buttons area is too dark. In the reference, the left portion of the
control strip shows the "Modern" dropdown box (a moderate grey color ~rgb(78,78,93)).
M-LIM shows a 4×2 button grid with dark inactive buttons and narrow gaps.

**Fix — two color changes in `Colours.h`:**

### Change 1: Brighten inactive algo buttons

The inactive button face color needs to be lighter to reduce the gap to the reference:

```cpp
// BEFORE:
const juce::Colour algoButtonInactive { 0xff303848 };  // rgb(48,56,72)
// AFTER:
const juce::Colour algoButtonInactive { 0xff3C4455 };  // rgb(60,68,85) — +12 units
```

### Change 2: Slightly increase control strip gradient brightness

To improve the knob area gap of 6-9 units:

```cpp
// BEFORE:
const juce::Colour controlStripTop    { 0xff575468 };  // rgb(87,84,104)
const juce::Colour controlStripBottom { 0xff454350 };  // rgb(69,67,80)
// AFTER:
const juce::Colour controlStripTop    { 0xff5C5870 };  // rgb(92,88,112) — +5R, +4G, +8B
const juce::Colour controlStripBottom { 0xff4A4858 };  // rgb(74,72,88)  — +5R, +5G, +8B
```

**Expected improvement:** control strip RMSE ~0.2–0.4pp.

## Produces
None

## Consumes
None

## Relevant Files
Modify: `src/ui/Colours.h` — `algoButtonInactive`, `controlStripTop`, `controlStripBottom`
Read: `src/ui/AlgorithmSelector.cpp` — to verify `algoButtonInactive` usage context

## Acceptance Criteria
- [ ] Run: build + screenshot + control strip RMSE → Expected: ≤ 20.1% (improvement from 20.17%)
- [ ] Run: full image RMSE → Expected: ≤ 20.74% (no regression from wave 18 baseline)
- [ ] Run: visual check → Expected: algo buttons slightly lighter (but still clearly inactive/dark); control strip background slightly brighter

## Tests
None

## Technical Details

In `src/ui/Colours.h`, locate and update the three constants:

```cpp
const juce::Colour algoButtonInactive  { 0xff303848 };
const juce::Colour controlStripTop    { 0xff575468 };
const juce::Colour controlStripBottom { 0xff454350 };
```

Change to:
```cpp
const juce::Colour algoButtonInactive  { 0xff3C4455 };
const juce::Colour controlStripTop    { 0xff5C5870 };
const juce::Colour controlStripBottom { 0xff4A4858 };
```

Build and measure control strip RMSE:
```bash
export CCACHE_DIR=/build-cache
cmake --build /workspace/M-LIM/build --target MLIM_Standalone -j$(nproc)

convert /reference-docs/reference-screenshots/prol2-main-ui.jpg \
    -crop 1712x1073+97+32 +repage -resize 900x500! /tmp/ref.png
pkill -f "Standalone/M-LIM" 2>/dev/null; sleep 1
DISPLAY=:99 /workspace/M-LIM/build/MLIM_artefacts/Release/Standalone/M-LIM &>/dev/null &
sleep 8; DISPLAY=:99 scrot /tmp/raw.png; pkill -f "Standalone/M-LIM"
convert /tmp/raw.png -crop 908x500+509+325 +repage -resize 900x500! /tmp/mlim.png

convert /tmp/mlim.png -crop 900x90+0+410 +repage /tmp/ctrl-mlim.png
convert /tmp/ref.png  -crop 900x90+0+410 +repage /tmp/ctrl-ref.png
compare -metric RMSE /tmp/ctrl-ref.png /tmp/ctrl-mlim.png /dev/null 2>&1
compare -metric RMSE /tmp/ref.png /tmp/mlim.png /dev/null 2>&1
```

If control RMSE worsens, revert algoButtonInactive change first, test without it.
If visual appearance of buttons looks too washed out/flat, reduce increment to 0xff383F50.

## Dependencies
None
