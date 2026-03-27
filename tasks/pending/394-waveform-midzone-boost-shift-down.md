# Task 394: Waveform Mid-Zone Brightness Boost — Shift Center Down to 58%

## Description

The waveform display background currently has two overlapping tent-shaped brightness
fills (mid-zone boost and center boost), both peaking at 50% of display height (y=250px).
Pixel analysis of the reference image shows the brightness peak zone is at ~60% height
(y=300px), not 50%.

**Measured pixel values at key heights (idle, no audio):**

| Height | M-LIM | Reference | Delta |
|--------|-------|-----------|-------|
| 40% (y=200) | `#6F748E` | `#68749B` | ~8 counts — GOOD |
| 60% (y=300) | `#5F688C` | `#909AB5` | ~50 counts — TOO DARK |

The current boost peaks at y=250 and falls off toward y=300, leaving the 56-70% zone
underbright. Shifting both tent centers from 50% to 58% moves the peak to y=290,
better covering the reference's bright zone at y=300.

Also increase the peak alpha of both tent fills slightly:
- Mid-zone boost: peak alpha 0.68 → 0.80
- Center tent boost: peak alpha 0.55 → 0.65

## Produces
None

## Consumes
None

## Relevant Files
Modify: `M-LIM/src/ui/WaveformDisplay.cpp` — the `drawBackground()` method, two tent fill blocks
Read: `M-LIM/src/ui/Colours.h` — reference for colour constants

## Acceptance Criteria
- [ ] Run: `cmake --build build --target MLIM_Standalone_Standalone -j$(nproc)` → Expected: compiles clean
- [ ] Run: launch standalone + screenshot, measure pixel at y=300 x=320 → Expected: brighter than `#5F688C`, closer to reference `#909AB5`
- [ ] Run: `compare -metric RMSE` wave region → Expected: ≤ 16.50% (improvement from 17.29%)
- [ ] Run: `compare -metric RMSE` full → Expected: ≤ 19.50% (no regression from 19.82%)

## Tests
None

## Technical Details

In `WaveformDisplay::drawBackground()`, locate the two tent fill blocks:

**Block 1 — Mid-zone brightness boost:**
```cpp
// BEFORE:
const float midTop   = area.getY() + area.getHeight() * 0.28f;
const float midMid   = area.getY() + area.getHeight() * 0.50f;
const float midBot   = area.getY() + area.getHeight() * 0.75f;
// ...alpha 0.68f peaks...

// AFTER:
const float midTop   = area.getY() + area.getHeight() * 0.36f;
const float midMid   = area.getY() + area.getHeight() * 0.58f;
const float midBot   = area.getY() + area.getHeight() * 0.82f;
// ...alpha 0.80f peaks...
```

**Block 2 — Center tent boost:**
```cpp
// BEFORE:
const float cTop = area.getY() + area.getHeight() * 0.32f;
const float cMid = area.getY() + area.getHeight() * 0.50f;
const float cBot = area.getY() + area.getHeight() * 0.68f;
// ...alpha 0.55f peak...

// AFTER:
const float cTop = area.getY() + area.getHeight() * 0.40f;
const float cMid = area.getY() + area.getHeight() * 0.58f;
const float cBot = area.getY() + area.getHeight() * 0.76f;
// ...alpha 0.65f peak...
```

If the wave RMSE worsens or y=200 pixel match degrades noticeably, try a smaller shift
(0.54 instead of 0.58) and report the measured values.

## Dependencies
None
