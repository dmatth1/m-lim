# Task 359: Algorithm Selector Background Color Matches Control Strip

## Description

The `AlgorithmSelector` component fills itself with `displayBackground: 0xff111118` (near-black),
which creates a visually distinct dark box in the control strip area. The reference Pro-L 2
algorithm selector blends more naturally with the control strip background.

**Evidence (pixel sampling 2026-03-27):**

At the bottom of the control strip (y=490 in 900×500 image), x=50 (in the STYLE/AlgorithmSelector column):
- Reference: `#34313C` = RGB(52, 49, 60) — dark purple-gray (control strip background)
- M-LIM: `#111118` = RGB(17, 17, 24) — near-black (`displayBackground`)

The difference is large: M-LIM is 65% darker than reference at those pixels, creating a visible
dark rectangle that conflicts with the control strip gradient background.

**Fix:**

In `AlgorithmSelector.cpp` (or wherever the background is painted), change the fill color from
`displayBackground` to a slightly lighter color that blends with the control strip:

Option A: Use `widgetBackground: 0xff222230` (slightly lighter, still dark)
Option B: Use `algoButtonInactive: 0xff303848` (matches inactive button color, blend seamlessly)
Option C: Use a new constant `algoSelectorBackground` ≈ `0xff28283A`

Recommended: **Option B** — use `algoButtonInactive` as the surrounding background, so the entire
selector looks like a unified dark panel of buttons.

Also consider removing the explicit background fill entirely and relying on the control strip
gradient, with only the button rectangles themselves having colored backgrounds.

## Produces
None

## Consumes
None

## Relevant Files
Modify: `M-LIM/src/ui/AlgorithmSelector.cpp` — change background fill color in paint()
Read: `M-LIM/src/ui/Colours.h` — background color options
Read: `M-LIM/src/ui/ControlStrip.cpp` — controlStripTop/Bottom gradient for context

## Acceptance Criteria
- [ ] Run: `cmake --build M-LIM/build -j$(nproc) 2>&1 | tail -3` → Expected: build succeeds
- [ ] Run: `source Scripts/ui-test-helper.sh && start_app && screenshot_safe "task-359-after.png" && stop_app` → Expected: screenshot captured
- [ ] Visually verify: Algorithm selector background blends more naturally with surrounding control strip gradient (no harsh dark box contrast)
- [ ] Run: RMSE comparison → Expected: full-image RMSE ≤ 22.08%

## Tests
None

## Technical Details

```bash
# Find the background paint call
grep -n "displayBackground\|fillRect\|fillRoundedRect\|background" /workspace/M-LIM/src/ui/AlgorithmSelector.cpp

# RMSE measurement after fix
convert /reference-docs/reference-screenshots/prol2-main-ui.jpg \
    -crop 1712x1073+97+32 +repage -resize 900x500! /tmp/ref.png
source Scripts/ui-test-helper.sh && start_app && sleep 2
scrot /tmp/t359-raw.png && stop_app
convert /tmp/t359-raw.png -crop 908x500+509+325 +repage -resize 900x500! /tmp/t359-mlim.png
compare -metric RMSE /tmp/ref.png /tmp/t359-mlim.png /dev/null 2>&1
```

## Dependencies
None
