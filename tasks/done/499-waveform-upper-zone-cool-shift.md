# Task 499: Waveform Upper Zone — Cool Colour Shift to Match Reference

## Description
Pixel analysis of the current build vs the Pro-L 2 reference shows a persistent colour
discrepancy in the upper waveform zone (top 10–15% of display):

- M-LIM at y≈50 (10% from top): RGB(44, 36, 38) — slightly warm (R > B)
- Reference at same position: RGB(39, 37, 42) — slightly cool (B > R)
- Delta: R+5, G-1, B-4 (M-LIM is too warm/reddish)

This warmth comes from `displayGradientTop = 0xff282020` (R=40, G=32, B=32) which has
a warm red-dominant cast. The reference top is more neutral-cool. Task-421 set this value
empirically but the reference shows a cooler upper zone.

**Fix approach**: Shift `displayGradientTop` toward neutral-to-cool:
- Current: `0xff282020` (R=40, G=32, B=32) — warm
- Proposed: `0xff201E24` (R=32, G=30, B=36) — slightly cool, matching reference direction

This should reduce the upper zone RMSE by bringing the R channel down (~5 units) and
the B channel up (~4 units), aligning with the reference (39,37,42) target.

**Important**: Measure Wave RMSE before and after. Accept only if Full RMSE ≤ current
baseline and Wave RMSE improves. If this worsens either metric, revert.

## Produces
None

## Consumes
None

## Relevant Files
Modify: `src/ui/Colours.h` — `displayGradientTop` constant (line ~22)
Read: `screenshots/task-409-rmse-results.txt` — previous baseline for comparison

## Acceptance Criteria
- [ ] Run: `grep 'displayGradientTop' src/ui/Colours.h` → Expected: shows `0xff201E24` (or close neutral-cool variant)
- [ ] Run: Build, launch, take crop, compare RMSE → Expected: Wave RMSE improves from current; Full RMSE does not regress beyond +0.3pp

## Tests
None

## Technical Details
The upper zone pixel at y=50 (10% from top of waveform area) is almost entirely determined
by the displayGradientTop colour since there is no idle fill at the very top. The upper idle
fill starts at 20% from top (task-450: uTop = area.getY() + area.getHeight() * 0.20f).

Reference colour target at upper zone: RGB(39, 37, 42) ≈ `#272529`
Gradient top should be close to this target.

Current value gives slightly warm (brownish) top zone that doesn't match the Pro-L 2's
neutral-to-cool dark background at the top of the waveform display.

**RMSE methodology for validation:**
```bash
# After building and launching:
convert /reference-docs/reference-screenshots/prol2-main-ui.jpg \
  -crop 1712x1073+97+32 +repage -resize 900x500! /tmp/ref.png
# M-LIM crop:
convert /tmp/screenshot.png -crop 908x500+509+325 +repage -resize 900x500! /tmp/mlim.png
# Wave region RMSE:
compare -metric RMSE <(convert /tmp/mlim.png -crop 640x500+0+0 +repage png:-) \
                     <(convert /tmp/ref.png  -crop 640x500+0+0 +repage png:-) /dev/null 2>&1
```

## Dependencies
None
