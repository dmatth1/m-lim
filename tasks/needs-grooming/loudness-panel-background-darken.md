# Task: Loudness Panel Background Darken

## Description
The loudness panel background zone (x=660-800 in the 900x500 comparison crop) is significantly lighter than the reference. Pixel analysis shows:
- Reference average: `#3A353F` (R=58, G=53, B=63)
- M-LIM current: `#454453` (R=69, G=68, B=83)
- Gap: M-LIM is ~11-20 units TOO BRIGHT — the panel reads as noticeably lighter than reference

The `loudnessPanelBackground` color constant needs to be darkened. Current value is `0xff464356` = (70,67,86). The histogram gradient colors (`loudnessHistogramTop` and `loudnessHistogramBottom`) which are `0xff484858` and `0xff404050` also contribute to the composite brightness.

Target: composite average should approach `#3A353F` = (58,53,63). This means reducing `loudnessPanelBackground` by approximately 10-15 units per channel (to something around `0xff383445` or darker).

## Produces
None

## Consumes
None

## Relevant Files
Modify: `src/ui/Colours.h` — `loudnessPanelBackground`, `loudnessHistogramTop`, `loudnessHistogramBottom`
Read: `src/ui/LoudnessPanel.cpp` — to understand how background is painted

## Acceptance Criteria
- [ ] Run: pixel color check after build `convert screenshots/task-loudness-panel-after.png -crop 140x340+660+40 +repage -resize 1x1! txt:-` → Expected: composite R ≤ 62 (down from 69)
- [ ] Run: `source Scripts/ui-test-helper.sh && start_app && screenshot screenshots/task-loudness-panel-after.png && stop_app` → Expected: visible darkening of loudness panel background visible in screenshot

## Technical Details
In `Colours.h`:
- `loudnessPanelBackground`: `0xff464356` → try `0xff363244` (darker by ~16 units)
- `loudnessHistogramTop`: `0xff484858` → try `0xff383848` (darker by ~16 units)
- `loudnessHistogramBottom`: `0xff404050` → try `0xff303040` (darker by ~16 units)

Measure the composite after each change with ImageMagick and adjust to hit the target composite average of ~(58, 53, 63).

Build only Standalone: `cmake --build build --target MLIM_Standalone -j$(nproc)`.

## Dependencies
None
