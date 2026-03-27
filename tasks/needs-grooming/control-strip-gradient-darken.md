# Task: Control strip gradient — darken to match reference (Control zone RMSE)

## Description
The control strip gradient (controlStripTop/Bottom in Colours.h) is consistently
brighter than the reference Pro-L 2 control strip across all sub-regions.

Pixel analysis of the Control zone (900×90 crop at y=410):
- Top label row (y=2–17): M-LIM avg #666374 (R=102, G=99, B=116) vs Ref #4B4B58 (R=75, G=75, B=88) — M-LIM +27R, +24G, +28B too bright
- Mid knob area (y=35–60): M-LIM avg #5D5B6C vs Ref #535568 — M-LIM ~+10 units brighter
- Bottom row (y=70–88): M-LIM avg #52515D vs Ref #424150 — M-LIM +16R, +16G, +13B too bright

Task-392 previously brightened the control strip by +13. The reference is still ~25 units
darker than M-LIM at the top label row. The strip needs to be darkened to close this gap.

## Produces
None

## Consumes
None

## Relevant Files
Modify: `M-LIM/src/ui/Colours.h` — controlStripTop and controlStripBottom constants (lines 68–69)

## Acceptance Criteria
- [ ] Run: build → Expected: compiles without error
- [ ] Run: Control zone RMSE (crop 900x90+0+410) → Expected: Control RMSE < 19.37% baseline
- [ ] Run: Full RMSE → Expected: Full ≤ 19.11% (no regression)

## Tests
None

## Technical Details
Current values in `Colours.h`:
```cpp
const juce::Colour controlStripTop    { 0xff696578 };  // +13 brightening from task-387 value
const juce::Colour controlStripBottom { 0xff565362 };  // +12 brightening from task-387 value
```

The top label-row gap is ~25 units R/G/B. To close half that gap (conservative, since
other elements on the strip contribute to the average), reduce by ~12–15 units:

**Proposed target:**
```cpp
const juce::Colour controlStripTop    { 0xff5A5769 };  // ~R-15/G-14/B-15 from current
const juce::Colour controlStripBottom { 0xff4A4856 };  // ~R-12/G-11/B-12 from current
```

Measure before and after:
```bash
convert /tmp/mlim.png -crop 900x90+0+410 +repage /tmp/z-ctrl.png
convert /tmp/ref.png  -crop 900x90+0+410 +repage /tmp/r-ctrl.png
compare -metric RMSE /tmp/z-ctrl.png /tmp/r-ctrl.png /dev/null 2>&1
```

If the first reduction improves RMSE, try a second step of -5 to -8 units.
If the RMSE worsens, revert and try a smaller reduction (-8/-7).

## Dependencies
None
