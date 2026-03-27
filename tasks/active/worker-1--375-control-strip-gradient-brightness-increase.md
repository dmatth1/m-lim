# Task 375: Control Strip — Increase Gradient Brightness

## Description

The control strip gradient background is ~20-25% darker than the reference.

**Pixel analysis (control strip, 900x90px at bottom):**

| Zone | Reference (R,G,B) | M-LIM (R,G,B) | Gap |
|------|-------------------|---------------|-----|
| Left knob area (x=0–200)     | 84, 85, 100 | 47–69, 39–67, 43–77 | ~30% dimmer |
| Center knob area (x=300–650) | 74–92, 74–97, 87–121 | 59–65, 57–63, 68–74 | ~25% dimmer |
| Right area (x=650–800)       | 80, 82, 103 | 59, 57, 68 | ~30% dimmer |

Current color constants:
- `controlStripTop`:    `0xff4A4756` — R=74, G=71, B=86
- `controlStripBottom`: `0xff38353F` — R=56, G=53, B=63

**Fix:** Brighten both gradient stops by ~12 luminosity units:
- `controlStripTop`:    `0xff4A4756` → `0xff575468`  (R=87, G=84, B=104)
- `controlStripBottom`: `0xff38353F` → `0xff454350`  (R=69, G=67, B=80)

**Expected improvement:** ~0.8–1.5 pp reduction in control strip RMSE (20.65% → ~19–20%).

**Note:** Task 374 also modifies `Colours.h` (different constants: loudnessHistogramTop/Bottom).
These can run in parallel; workers must edit only their specified constants.

## Produces
None

## Consumes
None

## Relevant Files
Modify: `src/ui/Colours.h` — change `controlStripTop` and `controlStripBottom`
Read: `src/ui/ControlStrip.cpp` — how the gradient is used in `paint()` (~lines 370–382)
Skip: `src/ui/LoudnessPanel.cpp` — uses separate histogram constants

## Acceptance Criteria
- [ ] Run: build + screenshot + control strip region RMSE → Expected: < 20.65% (wave 16 baseline)
- [ ] Run: full image RMSE → Expected: ≤ 21.22% (no regression)

## Tests
None

## Technical Details

Change in `Colours.h` only:
```cpp
// Before:
const juce::Colour controlStripTop    { 0xff4A4756 };
const juce::Colour controlStripBottom { 0xff38353F };

// After:
const juce::Colour controlStripTop    { 0xff575468 };  // +13 brightness
const juce::Colour controlStripBottom { 0xff454350 };  // +13 brightness
```

If these values cause regression, try +8 instead:
- `controlStripTop:    0xff52504E`
- `controlStripBottom: 0xff403E4A`

Build and measure:
```bash
export CCACHE_DIR=/build-cache
cmake --build /workspace/M-LIM/build --target MLIM_Standalone_Standalone -j$(nproc)

convert /reference-docs/reference-screenshots/prol2-main-ui.jpg \
    -crop 1712x1073+97+32 +repage -resize 900x500! /tmp/ref.png

pkill -f "Standalone/M-LIM" 2>/dev/null; sleep 1
DISPLAY=:99 /workspace/M-LIM/build/MLIM_artefacts/Release/Standalone/M-LIM &>/dev/null &
sleep 8
DISPLAY=:99 scrot /tmp/raw.png
pkill -f "Standalone/M-LIM"

convert /tmp/raw.png -crop 908x500+509+325 +repage -resize 900x500! /tmp/mlim.png

# Control strip RMSE
convert /tmp/mlim.png -crop 900x90+0+410 +repage /tmp/ctrl-mlim.png
convert /tmp/ref.png  -crop 900x90+0+410 +repage /tmp/ctrl-ref.png
compare -metric RMSE /tmp/ctrl-ref.png /tmp/ctrl-mlim.png /dev/null 2>&1

# Full RMSE
compare -metric RMSE /tmp/ref.png /tmp/mlim.png /dev/null 2>&1
```

## Dependencies
None
