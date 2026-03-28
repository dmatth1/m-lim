# Task: Waveform Center Zone Still Too Dark vs Reference

## Description
The waveform display center region (the mid-height area ~50% down) is significantly darker than the Pro-L 2 reference:

Pixel comparison at x=350, Y=250 (waveform center):
- Current: srgb(89,91,113) — dark blue-gray
- Reference: srgb(145,157,192) — much brighter blue-gray

The reference waveform at idle shows a composite appearance where the input/output fills create a bright blue-gray zone in the center-to-lower portion. Our waveform center is ~60 units too dark in all channels.

While there have been multiple prior tasks tweaking idle gradients (task-422, task-450, task-499, task-522), the cumulative result is still significantly below reference brightness in the center band. The idle fills need further brightening in the 40-70% height range.

## Produces
None

## Consumes
None

## Relevant Files
Modify: `src/ui/WaveformDisplay.cpp` — idle fill gradients in `drawBackground()` (lines 292-393), particularly the mid-zone tent fills
Modify: `src/ui/Colours.h` — `waveformIdleMidFill` (0xff828AA5) and `waveformIdleLowFill` (0xffA8A8B8)
Read: `src/ui/WaveformDisplay.h` — display constants

## Acceptance Criteria
- [ ] Run: `cmake --build build --target MLIM_Standalone -j$(nproc)` → Expected: builds successfully
- [ ] Run: screenshot RMSE comparison → Expected: waveform RMSE improves from ~22.5%

## Tests
None

## Technical Details
- The center zone tent fill uses `waveformIdleMidFill` (0xff828AA5 = 130,138,165) at peak alpha 0.52
- Composite result: background gradient (~32,30,36) + fill (130,138,165 * 0.52) ≈ (99,102,121) — still 40-50 units short of reference (145,157,192)
- Options:
  1. Increase mid tent alpha from 0.52 to 0.75-0.80
  2. Brighten `waveformIdleMidFill` from 0xff828AA5 to ~0xffA0AAC0
  3. Increase center tent alpha from 0.40 to 0.60
  4. Extend the coverage of lower idle fill to start higher
- Be careful not to over-brighten the top zone (0-30% height) which should remain near-black

## Dependencies
None
