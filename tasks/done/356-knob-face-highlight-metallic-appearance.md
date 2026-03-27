# Task 356: Increase Knob Face Highlight for Metallic Appearance

## Description

The rotary knob face highlight color `knobFaceHighlight: 0xff808080` (50% gray) produces knobs
that look flat/matte. The reference Pro-L 2 shows knobs with a bright metallic highlight at the
top-left of the face, giving a convincing 3D sphere appearance.

**Evidence from reference pixel sampling (2026-03-27):**

At the control strip region (y≈460 in the 900×500 image), the reference shows bright pixels
of approximately `#C8C8D8` to `#DBDBE7` at the knob face highlight positions. Current M-LIM
uses `0xff808080` = `#808080` (50% gray) for the highlight, which is:
- Reference highlight: ~`#DBDBE7` = RGB(219, 219, 231) ≈ 86% brightness, slight blue tint
- M-LIM highlight: `#808080` = RGB(128, 128, 128) = 50% brightness, neutral gray

**Fix:**

In `M-LIM/src/ui/Colours.h`:

```cpp
// Change:
const juce::Colour knobFaceHighlight{ 0xff808080 };  // neutral light gray highlight

// To:
const juce::Colour knobFaceHighlight{ 0xffC0C0D0 };  // brighter blue-tinted highlight (matches reference metallic sheen)
```

The value `0xffC0C0D0` = RGB(192, 192, 208):
- 75% brightness (vs current 50%, reference ~86%)
- Slight blue tint (matches reference's metallic look)

This brings the radial gradient from `knobFaceShadow` (0xff303030, 18%) to `knobFaceHighlight`
(0xffC0C0D0, 75%) closer to the reference's range (18% → 86%).

## Produces
None

## Consumes
None

## Relevant Files
Modify: `M-LIM/src/ui/Colours.h` — change `knobFaceHighlight` constant

## Acceptance Criteria
- [ ] Run: `cmake --build M-LIM/build -j$(nproc) 2>&1 | tail -3` → Expected: build succeeds
- [ ] Run: `source Scripts/ui-test-helper.sh && start_app && screenshot_safe "task-356-after.png" && stop_app` → Expected: screenshot captured
- [ ] Run: visually inspect `screenshots/task-356-after.png` → Expected: knob faces appear more metallic/shiny with a brighter lit-side highlight
- [ ] Run: RMSE comparison → Expected: full-image RMSE ≤ 22.08%; control strip sub-region RMSE not increased vs task-355 baseline

## Tests
None

## Technical Details

```bash
# RMSE measurement (same methodology as task-355)
convert /reference-docs/reference-screenshots/prol2-main-ui.jpg \
    -crop 1712x1073+97+32 +repage -resize 900x500! /tmp/ref.png
source Scripts/ui-test-helper.sh && start_app && sleep 2
scrot /tmp/t356-raw.png && stop_app
convert /tmp/t356-raw.png -crop 908x500+509+325 +repage -resize 900x500! /tmp/t356-mlim.png
compare -metric RMSE /tmp/ref.png /tmp/t356-mlim.png /dev/null 2>&1
```

If this change INCREASES full-image RMSE by more than 0.20%, revert to `0xff808080`.

## Dependencies
None (can run in parallel with task 355)
