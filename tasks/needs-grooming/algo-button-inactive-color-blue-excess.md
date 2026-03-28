# Task: Reduce algoButtonInactive Blue Excess to Match Reference

## Description

Pixel analysis of the algorithm selector buttons (TR, PU, DY, AG, AR, BU, SA, MO)
shows the inactive button background color has too much blue compared to reference:

- M-LIM current `algoButtonInactive = 0xff545870` = (84, 88, 112)
- Area average at buttons (170x80+0+407 crop, task-409): (86, 89, 106)
- Reference area average (task-409): (84, 84, 99)
- Gap: G+5, B+7 (buttons are too saturated blue)

**Fix**: Reduce the blue and green channels in `algoButtonInactive` to better match
the reference color (84, 84, 99):

```cpp
// Colours.h — single line change
const juce::Colour algoButtonInactive { 0xff545463 };  // R=84, G=84, B=99 — matches reference (was 0xff545870)
```

This is a single-line Colours.h change. No other files need modification.

**Why this differs from task-409**: Task-409 increased algoButtonInactive from (52,54,62)
to (84,88,112). The reference is (84,84,99). The B channel overshot by 13 and G by 4.
The fix corrects only those two channels back to the reference values.

**Expected effect**:
- Algo buttons will appear slightly less blue/saturated
- Control strip RMSE should improve (buttons cover ~17% of control strip)
- No other UI areas affected (this color is used only for algorithm buttons and the
  ADVANCED panel background in ControlStrip::paint())

**ADVANCED panel impact**: The ADVANCED narrow vertical strip (kAdvancedBtnW=18px) in
ControlStrip::paint() also uses `algoButtonInactive` for its background fill. Adjusting
this color will also affect the ADVANCED strip appearance.

## Produces
None

## Consumes
None

## Relevant Files
Modify: `src/ui/Colours.h` — change algoButtonInactive from 0xff545870 to 0xff545463
Read: `screenshots/task-409-rmse-results.txt` — reference pixel values and area measurement

## Acceptance Criteria
- [ ] Run: `convert /tmp/mlim.png -crop 170x80+0+407 +repage txt:- | awk -F'[(),: ]+' '{r+=$4; g+=$5; b+=$6; n++} END {printf "(%.0f, %.0f, %.0f)\n", r/n, g/n, b/n}'` → Expected: values within ±5 of (84,84,99)
- [ ] Run: control strip RMSE compare → Expected: ≤ 22.42% (task-354 baseline, no regression)
- [ ] Run: Full RMSE compare → Expected: ≤ 18.95% (task-409 baseline, no regression)

## Tests
None

## Technical Details
- Single line change in Colours.h
- `algoButtonInactive = 0xff545463`: 0x54=84 (R), 0x54=84 (G), 0x63=99 (B)
- The reference (84,84,99) has R=G and the B channel is 15 lower (neutraler blue-grey)

## Dependencies
None
