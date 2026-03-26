# Task 329: Revert Control Strip Knob Size (kKnobRowH 72→56, kControlStripH 108→92)

## Description
Task 318 increased the control strip knob row height from 56px to 72px (kKnobRowH) and the
total control strip height from 92px to 108px (kControlStripH). The RMSE remeasure (task 324)
confirmed this caused a regression:
  - Waveform RMSE: 20.00% → 22.08% (+2.08 pp)
  - Control strip RMSE: 22.57% → 23.40% (+0.83 pp)

The RMSE was measurably better with the smaller knob sizes. Reverting restores the waveform
height from 368px back to 384px, which better aligns the dB grid lines with their positions
in the reference (prol2-main-ui.jpg rescaled to 900x500).

Note: The reference Pro-L 2 control strip appears to be ~120px at 1x scale, so neither 92 nor
108 is perfect. However, 92px gave lower RMSE in practice because it produces grid line
y-positions that better align pixel-for-pixel with the scaled reference image.

## Produces
None

## Consumes
None

## Relevant Files
Modify: `M-LIM/src/PluginEditor.h` — change `kControlStripH` from 108 to 92
Modify: `M-LIM/src/ui/ControlStrip.cpp` — change `kKnobRowH` from 72 to 56

## Acceptance Criteria
- [ ] Run: `cmake --build M-LIM/build --config Release -j$(nproc) 2>&1 | tail -3` → Expected: build succeeds, exit 0
- [ ] Run: `source Scripts/ui-test-helper.sh && start_app && screenshot "screenshots/task-329-after.png" && stop_app` → Expected: screenshot saved
- [ ] Visual: Control strip is slightly shorter (knobs more compact); waveform area is taller

## Tests
None

## Technical Details
Two changes needed:

**1. In `M-LIM/src/PluginEditor.h`:**
```cpp
// BEFORE:
static constexpr int kControlStripH  = 108;

// AFTER:
static constexpr int kControlStripH  = 92;
```

**2. In `M-LIM/src/ui/ControlStrip.cpp`** (the anonymous namespace at the top of the file):
```cpp
// BEFORE:
static constexpr int kKnobRowH    = 72;

// AFTER:
static constexpr int kKnobRowH    = 56;
```

These are the only changes needed.

## Dependencies
Requires task 328 (revert output meter width) to be complete first, so all layout changes
are applied together before the RMSE remeasure.
