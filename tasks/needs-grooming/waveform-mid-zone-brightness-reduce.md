# Task: Waveform Mid Zone Brightness Reduce

## Description
The mid waveform zone (y=30-50% of the display, y=150-250 in the 900x500 crop) is too bright compared to the reference. Pixel analysis:
- Reference average: `#5C637F` (R=92, G=99, B=127)
- M-LIM current: `#696B82` (R=105, G=107, B=130)
- Gap: M-LIM is ~13R, 8G, 3B too bright in the mid zone

The source of this over-brightness is the mid-zone tent pass in `WaveformDisplay::drawBackground`. The tent centered at y=58% extends from y=36% to y=82% with peak alpha 0.68. This passes the 30-50% height range on its rising slope.

Additionally the center tent (40-76%, alpha 0.52) contributes brightness in this zone.

Reduce the peak alphas of the mid-zone tent and center tent to bring the mid zone composite down by ~10 units:
- Mid-zone tent peak: 0.68 → ~0.52
- Center tent peak: 0.52 → ~0.40

Verify by measuring the composite after building.

## Produces
None

## Consumes
None

## Relevant Files
Modify: `src/ui/WaveformDisplay.cpp` — mid-zone tent alphas (lines ~340-380)

## Acceptance Criteria
- [ ] Run pixel check: `convert screenshots/task-wave-mid-after.png -crop 540x100+0+150 +repage -resize 1x1! txt:-` → Expected: R channel ≤ 97 (down from 105), composite closer to #5C637F
- [ ] Run RMSE: `compare -metric RMSE <ref-crop> <mlim-crop> /dev/null 2>&1` → Expected: Wave zone RMSE improves

## Technical Details
In `WaveformDisplay.cpp` mid-zone tent block (~lines 340-370):
```cpp
// Change mid-zone tent alphas:
// Rising half: 0.0f → 0.52f (was 0.68f)
midFill.withAlpha (0.52f),  0.0f, midMid,
// Falling half: 0.52f → 0.0f (was 0.68f)
midFill.withAlpha (0.52f),  0.0f, midMid,
```

In center tent block (~lines 375-405):
```cpp
// Rising half: 0.0f → 0.40f (was 0.52f)
cCol.withAlpha (0.40f),  0.0f, cMid,
// Falling half: 0.40f → 0.0f (was 0.52f)
cCol.withAlpha (0.40f),  0.0f, cMid,
```

These changes reduce the blue-grey overlay in the 30-50% height band. Build Standalone only: `cmake --build build --target MLIM_Standalone -j$(nproc)`.

## Dependencies
None
