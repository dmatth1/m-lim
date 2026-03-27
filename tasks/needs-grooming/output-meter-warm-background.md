# Task: Output Meter Track — Warm Up from Cool Blue-Purple to Warm Reddish

## Description
The output meter bar track background renders as (40, 39, 51) = cool blue-purple (`#282733`) in idle
state, while the reference shows (40, 27, 31) = warm dark reddish (`#281B1F`) at the same positions.

**Root cause — two compounding issues:**

1. `barTrackBackground = 0xff1C1820` (28, 24, 32) has a blue bias (B > R), and
2. the idle structural gradient in `LevelMeter::drawChannel()` adds `meterSafe`-derived colors at
   15% alpha — `meterSafe = #6879A0` (104, 121, 160) is a cold blue, which pushes B up further.

Combined result at y=150 (mid-meter): (40, 39, 51) vs reference (40, 27, 31) — G and B are 12–20
units too high.

**Fix — two coordinated changes:**

**A. Change `barTrackBackground` in `Colours.h`** from `0xff1C1820` to `0xff231417` (35, 20, 23)
— a warm dark reddish-maroon that, combined with the reduced idle gradient, produces approximately
(40, 27, 32) which closely matches the reference (40, 27, 31).

Derivation:  with new base (35, 20, 23) and idle alpha 0.07f, the idle gradient at mid-bar
contributes ~(112, 126, 158) at 7% = (7.8, 8.8, 11.1) — net: (35×0.93+7.8, 20×0.93+8.8,
23×0.93+11.1) = (40.4, 27.4, 32.5) ≈ (40, 27, 33). Very close to reference.

**B. Reduce idle structural gradient alpha in `LevelMeter.cpp`** from `0.15f` to `0.07f` for all
four alpha values in the idle gradient block. The gradient still renders visually — it's just
less dominant so the warm barTrackBackground controls the overall hue.

Note: `barTrackBackground` is also used by `GainReductionMeter.cpp` (12px wide GR bar). Changing
it affects GR meter too, but the GR meter's 12px width has minimal RMSE impact.

## Produces
None

## Consumes
None

## Relevant Files
Modify: `M-LIM/src/ui/Colours.h` — change `barTrackBackground` from `0xff1C1820` to `0xff231417`
Modify: `M-LIM/src/ui/LevelMeter.cpp` — change idle gradient alpha values from `0.15f` to `0.07f`
Read: `M-LIM/src/ui/GainReductionMeter.cpp` — confirm barTrackBackground usage is incidental

## Acceptance Criteria
- [ ] Run: `grep -n "barTrackBackground" M-LIM/src/ui/Colours.h` → Expected: value is `0xff231417`
- [ ] Run: `grep -n "0.15f" M-LIM/src/ui/LevelMeter.cpp | grep -i "alpha\|idle"` → Expected: no matches (all changed to 0.07f)
- [ ] Build, launch, capture screenshot, compute right panel RMSE using correct methodology:
  ```
  # Prepare reference:
  convert /reference-docs/reference-screenshots/prol2-main-ui.jpg \
      -crop 1712x1073+97+32 +repage -resize 900x500! /tmp/ref.png
  # Capture M-LIM and crop:
  # [launch on Xvfb, scrot, crop 908x500+509+325, resize 900x500]
  compare -metric RMSE \
      <(convert /tmp/ref.png -crop 100x400+800+50 +repage png:-) \
      <(convert /tmp/task-mlim.png -crop 100x400+800+50 +repage png:-) \
      /dev/null 2>&1
  ```
  → Expected: right panel RMSE ≤ 28.00% (improvement from current 29.26%)

## Tests
None

## Technical Details

**Exact change to Colours.h:**
```cpp
// BEFORE:
const juce::Colour barTrackBackground { 0xff1C1820 };
// AFTER:
const juce::Colour barTrackBackground { 0xff231417 };  // warm dark reddish-maroon (matches ref #281B1F)
```

**Exact change to LevelMeter.cpp idle gradient block:**
```cpp
// Find the block starting with: "Idle structural gradient"
// Change ALL four .withAlpha(0.15f) calls to .withAlpha(0.07f):
MLIMColours::meterDanger.withAlpha (0.07f),
MLIMColours::meterSafe.darker (0.3f).withAlpha (0.07f),
MLIMColours::meterWarning.withAlpha (0.07f),
MLIMColours::meterSafe.brighter (0.15f).withAlpha (0.07f),
```

**Colour math validation:**
- Reference target at mid-meter: (40, 27, 31) = `#281B1F`
- New barTrackBackground: (35, 20, 23)
- Idle gradient mid-bar contribution at 7%: ~(7.8, 8.8, 11.1)
- Net result: (35×0.93 + 7.8, 20×0.93 + 8.8, 23×0.93 + 11.1) = (40.4, 27.4, 32.5) ≈ (40, 27, 33)

## Dependencies
None
