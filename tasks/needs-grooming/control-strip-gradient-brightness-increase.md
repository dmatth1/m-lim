# Task: Control Strip — Increase Gradient Brightness

## Description

The control strip gradient background is ~20-25% darker than the reference. This creates a
persistent RMSE gap across the entire bottom control area.

**Pixel analysis (control strip, 900x90px at bottom):**

| Zone | Reference (R,G,B) | M-LIM (R,G,B) | Gap |
|------|-------------------|---------------|-----|
| Left knob area (x=0–200)     | 84, 85, 100 | 47–69, 39–67, 43–77 | ~30% dimmer |
| Center knob area (x=300–650) | 74–92, 74–97, 87–121 | 59–65, 57–63, 68–74 | ~25% dimmer |
| Right area (x=650–800)       | 80, 82, 103 | 59, 57, 68 | ~30% dimmer |

Current color constants:
- `controlStripTop`:    `0xff4A4756` — R=74, G=71, B=86
- `controlStripBottom`: `0xff38353F` — R=56, G=53, B=63

Reference measured knob area average: R≈80-82, G≈81-83, B≈97-101
→ Target: boost top by ~12 units per channel, boost bottom by ~12 units.

**Fix:** Brighten both gradient stops by ~12 luminosity units:

- `controlStripTop`:    `0xff4A4756` → `0xff575468` — R=87, G=84, B=104
- `controlStripBottom`: `0xff38353F` → `0xff454350` — R=69, G=67, B=80

This preserves the blue-gray hue shift but increases overall luminosity to better match
the reference's medium-dark cool-gray strip.

**Expected improvement:** ~0.8–1.5 pp reduction in control strip RMSE (20.65% → ~19–20%).

**Risk assessment:** Low. Pure brightness increase of an existing gradient. No structural
changes. The algo buttons and knob faces draw on top of this gradient and will be unaffected
since they use fixed colors.

## Produces
None

## Consumes
None

## Relevant Files
Modify: `M-LIM/src/ui/Colours.h` — change `controlStripTop` and `controlStripBottom`
Read: `M-LIM/src/ui/ControlStrip.cpp:370-382` — how the gradient is used in `paint()`

## Acceptance Criteria
- [ ] Run: screenshot + full RMSE → Expected: ≤ 21.22% (no regression from wave 16 baseline)
- [ ] Run: control strip region RMSE → Expected: < 20.65% (current baseline)

## Tests
None

## Technical Details

Change in `Colours.h` only:
```cpp
// Before:
const juce::Colour controlStripTop    { 0xff4A4756 };  // brightened top
const juce::Colour controlStripBottom { 0xff38353F };  // brightened bottom

// After:
const juce::Colour controlStripTop    { 0xff575468 };  // further brightened — closer to ref ~36% brightness
const juce::Colour controlStripBottom { 0xff454350 };  // further brightened — ≈ 26% brightness
```

If these values cause regression, fall back to +8 instead of +12:
- `controlStripTop:    0xff52504E` → try `0xff525060`
- `controlStripBottom: 0xff403E4A`

Measurement:
```bash
convert /tmp/mlim.png -crop 900x90+0+410 +repage /tmp/ctrl-mlim.png
convert /tmp/ref.png  -crop 900x90+0+410 +repage /tmp/ctrl-ref.png
compare -metric RMSE /tmp/ctrl-ref.png /tmp/ctrl-mlim.png /dev/null 2>&1
```

## Dependencies
None
