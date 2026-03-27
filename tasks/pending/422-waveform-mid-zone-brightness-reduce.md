# Task 422: Waveform Mid Zone Brightness Reduce

## Description
The mid waveform zone (y=30–50% height, y=150–250 in the 900x500 crop) is too bright
compared to the reference.

Pixel analysis:
- Reference avg: `#5C637F` (R=92, G=99, B=127)
- M-LIM current: `#696B82` (R=105, G=107, B=130)
- Gap: ~13R, 8G, 3B too bright

The mid-zone tent pass in `WaveformDisplay::drawBackground` (centered at y=58%, rising
from 36% to 82%, peak alpha 0.68) covers the 30–50% range on its rising slope. The center
tent (40–76%, alpha 0.52) also contributes.

Reduce both tent peak alphas:
- Mid-zone tent: 0.68 → 0.52
- Center tent: 0.52 → 0.40

## Produces
None

## Consumes
None

## Relevant Files
Modify: `src/ui/WaveformDisplay.cpp` — mid-zone tent alphas (lines ~339–374)

## Acceptance Criteria
- [ ] Run: `cmake --build build --target MLIM_Standalone_Standalone -j$(nproc) 2>&1 | tail -3` → Expected: build succeeds, exit 0
- [ ] Run: Wave zone RMSE (crop 600x400+150+50) → Expected: improvement or no regression vs prior baseline
- [ ] Run: Full RMSE → Expected: ≤ 19.50%

## Tests
None

## Technical Details
In `WaveformDisplay.cpp` mid-zone tent block (~lines 339–355):
```cpp
// Rising half: alpha 0.68f → 0.52f
midFill.withAlpha (0.52f),  0.0f, midMid,
// Falling half: alpha 0.68f → 0.52f
midFill.withAlpha (0.52f),  0.0f, midMid,
```

Center tent block (~lines 362–378):
```cpp
// Rising half: alpha 0.52f → 0.40f
cCol.withAlpha (0.40f),  0.0f, cMid,
// Falling half: alpha 0.52f → 0.40f
cCol.withAlpha (0.40f),  0.0f, cMid,
```

Build Standalone only: `cmake --build build --target MLIM_Standalone_Standalone -j$(nproc)`.

## Dependencies
Requires task 421
