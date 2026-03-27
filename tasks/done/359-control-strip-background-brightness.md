# Task 359: Control Strip Background Colour Brightness

## Description

The control strip knob area (x=150–750, y=405–465) shows a measurable brightness gap:

- M-LIM average colour: #37353D (≈ 22% brightness)
- Pro-L 2 reference:   #595B6D (≈ 36% brightness)

M-LIM's control strip is ~14% too dark. The current gradient colours in `Colours.h` are:
```cpp
const juce::Colour controlStripTop    { 0xff3B3840 };  // ≈ 23% brightness
const juce::Colour controlStripBottom { 0xff282530 };  // ≈ 16% brightness
```

Brightening these two constants will reduce the control strip sub-region RMSE by pulling
the average background colour closer to the Pro-L 2 reference.

## Produces
None

## Consumes
None

## Relevant Files
Modify: `src/ui/Colours.h` — `controlStripTop` and `controlStripBottom` constants
Read: `src/ui/ControlStrip.cpp` — to confirm which component uses these colours

## Acceptance Criteria
- [ ] Run: `cmake --build build -j$(nproc) 2>&1 | grep -c "error:"` → Expected: `0`
- [ ] Run: control strip RMSE (900x60+0+440) →
  Expected: ≤ 22.00% (improvement from 22.42% baseline)
- [ ] Run: full image RMSE → Expected: ≤ 22.08% (no regression)

## Tests
None

## Technical Details

In `src/ui/Colours.h`, change:
```cpp
const juce::Colour controlStripTop    { 0xff3B3840 };
const juce::Colour controlStripBottom { 0xff282530 };
```
to:
```cpp
const juce::Colour controlStripTop    { 0xff4A4756 };  // ≈ 29% brightness — closer to ref ~36%
const juce::Colour controlStripBottom { 0xff38353F };  // ≈ 22% brightness
```

The target background brightness is ~25–27% (the reference ~36% includes silver knob faces
which are significantly brighter, so the background itself needs to be ~25%).

If brightening causes a regression (worsens RMSE), tune back toward midpoint:
- Safe fallback top: 0xff424050 (≈ 26%)
- Safe fallback bottom: 0xff302E3A (≈ 19%)

**RMSE measurement (CORRECT methodology):**
```bash
convert /reference-docs/reference-screenshots/prol2-main-ui.jpg \
    -crop 1712x1073+97+32 +repage -resize 900x500! screenshots/task-359-ref.png

compare -metric RMSE \
  <(convert screenshots/task-359-mlim.png -crop 900x60+0+440 +repage png:-) \
  <(convert screenshots/task-359-ref.png  -crop 900x60+0+440 +repage png:-) \
  /dev/null 2>&1
```

Save full results (full image + all sub-regions) to `screenshots/task-359-rmse-results.txt`.

## Dependencies
None
