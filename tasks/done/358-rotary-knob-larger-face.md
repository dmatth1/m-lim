# Task 358: Increase RotaryKnob Visual Size

## Description

In the control strip, each RotaryKnob component is allocated a slot of ~150×56 px.
`RotaryKnob::paint()` currently reserves `labelH = 13.0f` above and `valueH = 13.0f`
below the knob circle, leaving only `56 - 26 = 30 px` for the knob face.

With `radius = 30/2 - 4 = 11 px` and `faceRadius = 11 × 0.8 = 8.8 px`, the rendered
knob face is only ~18 px in diameter — visually very small against Pro-L 2's reference
knobs (≈ 38–45 px diameter).

Fix: reduce the label/value text area so the knob circle gets more vertical room, making
knobs appear closer to the Pro-L 2 reference.

## Produces
None

## Consumes
None

## Relevant Files
Modify: `src/ui/RotaryKnob.cpp` — `paint()` method: reduce `labelH` and `valueH` constants
Modify: `src/ui/RotaryKnob.cpp` — `resized()` method: update slider bounds to match

## Acceptance Criteria
- [ ] Run: `cmake --build build -j$(nproc) 2>&1 | grep -c "error:"` → Expected: `0`
- [ ] Run: visual inspection — knob circles in control strip must be visually larger than
  before (diameter ≥ 38 px given 56 px row height)
- [ ] Run: control strip sub-region (900x60+0+440) RMSE →
  Expected: ≤ 22.42% (no regression from baseline); ideally improvement toward 21%

## Tests
None

## Technical Details

In `src/ui/RotaryKnob.cpp`, in the `paint()` method, change:
```cpp
const float labelH     = 13.0f;
const float valueH     = 13.0f;
```
to:
```cpp
const float labelH     = 10.0f;
const float valueH     =  8.0f;
```

This gives `textHeight = 18`, so `knobSize = jmin(150, 56 - 18) = 38 px` — more than
double the previous 30 px. The effective `faceRadius = (38/2 - 4) × 0.8 = 11.2 px`, giving
a face diameter of ~22 px (still smaller than Pro-L 2 but noticeably larger).

In the `resized()` method (used by the internal `Slider`), the same constants appear:
```cpp
const float labelH     = 13.0f;
const float textHeight = labelH + 13.0f;
```
Change to match:
```cpp
const float labelH     = 10.0f;
const float textHeight = labelH + 8.0f;
```

**Also update the arc track pen width** from `2.0f` to `2.5f` in `paint()` for both the
track background arc and value arc `strokePath` calls, to make the arc ring slightly thicker
and more visible at the larger knob size.

**RMSE measurement (CORRECT methodology):**
```bash
convert /reference-docs/reference-screenshots/prol2-main-ui.jpg \
    -crop 1712x1073+97+32 +repage -resize 900x500! screenshots/task-358-ref.png

compare -metric RMSE \
  <(convert screenshots/task-358-mlim.png -crop 900x60+0+440 +repage png:-) \
  <(convert screenshots/task-358-ref.png  -crop 900x60+0+440 +repage png:-) \
  /dev/null 2>&1
```

Save full results to `screenshots/task-358-rmse-results.txt`.

## Dependencies
None
