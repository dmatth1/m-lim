# Task 355: Enlarge Rotary Knob Faces in Control Strip

## Description

The rotary knobs in the control strip are too small. The knob face diameter is currently ~17.6px
(arc diameter ~22px), which appears as tiny dark dots in screenshots. The reference Pro-L 2 shows
knob faces approximately 25–30px in diameter — about 60–70% larger.

**Root cause (confirmed by pixel analysis, 2026-03-27):**

In `RotaryKnob.cpp`, the layout allocates:
- `labelH = 13.0f` (per-knob label above face)
- `valueH = 13.0f` (min/current/max values below face)
- `textHeight = labelH + valueH = 26.0f`
- `knobSize = jmin(width, height - textHeight)` = `jmin(150, 56 - 26)` = 30px
- `radius = knobSize * 0.5f - 4.0f` = 15 - 4 = **11px** (arc radius)
- `faceRadius = radius * 0.80f` = **8.8px** (face circle, diameter = **17.6px**)

**Fix:**

Reduce the text row heights and border padding in `RotaryKnob.cpp` to give more space to the
knob face:

1. `labelH`: 13.0f → **10.0f**
2. `valueH`: 13.0f → **10.0f**
3. Border subtraction in radius calculation: `-4.0f` → **-2.0f**

Expected result after change:
- `textHeight` = 20px (was 26px)
- `knobSize` = min(150, 56-20) = **36px** (was 30px)
- `radius` = 36/2 - 2 = **16px** (was 11px)
- `faceRadius` = 16 × 0.80 = **12.8px**, diameter = **25.6px** (was 17.6px, +45%)

Also increase the arc stroke width from 2.0px to **2.5px** for better arc visibility at the larger
size.

The font sizes stay the same (9pt label, 10pt value, 7pt min/max) — they fit in 10px rows.

## Produces
None

## Consumes
None

## Relevant Files
Modify: `M-LIM/src/ui/RotaryKnob.cpp` — change labelH, valueH, border, and arc stroke width
Read: `M-LIM/src/ui/Colours.h` — reference color constants (no change needed)
Read: `M-LIM/src/ui/ControlStrip.cpp` — verify kKnobRowH=56 is sufficient (no change expected)

## Acceptance Criteria
- [ ] Run: `cmake --build M-LIM/build -j$(nproc) 2>&1 | tail -3` → Expected: build succeeds, exit 0
- [ ] Run: `source Scripts/ui-test-helper.sh && start_app && screenshot_safe "task-355-after.png" && stop_app` → Expected: screenshot captured
- [ ] Run: visually inspect `screenshots/task-355-after.png` → Expected: rotary knobs in control strip appear noticeably larger (face diameter visibly bigger than tiny dots)
- [ ] Run: RMSE comparison using correct methodology (see Technical Details) → Expected: full-image RMSE ≤ 22.08% and control strip sub-region RMSE improved vs current

## Tests
None

## Technical Details

RMSE measurement methodology (CORRECT):
```bash
# Reference crop
convert /reference-docs/reference-screenshots/prol2-main-ui.jpg \
    -crop 1712x1073+97+32 +repage -resize 900x500! /tmp/ref.png

# M-LIM capture
source Scripts/ui-test-helper.sh
start_app
sleep 2
scrot /tmp/task355-raw.png
stop_app
convert /tmp/task355-raw.png -crop 908x500+509+325 +repage -resize 900x500! /tmp/task355-mlim.png

# Full image RMSE
compare -metric RMSE /tmp/ref.png /tmp/task355-mlim.png /dev/null 2>&1

# Control strip sub-region
compare -metric RMSE \
  <(convert /tmp/ref.png -crop 900x60+0+440 +repage png:-) \
  <(convert /tmp/task355-mlim.png -crop 900x60+0+440 +repage png:-) \
  /dev/null 2>&1
```

Baseline (task-354, correct methodology):
- Full image:    22.08%
- Control strip: 22.42% (task-352 baseline — note: fresh measurement shows ~70% discrepancy
  likely due to knob size; the 22.42% baseline was measured from a run with different app state)

**Min/max label font sizes** in the value row: uses `kFontSizeSmall - 1.0f = 8.0f`. Verify these
labels still render legibly after reducing valueH to 10px.

## Dependencies
None
