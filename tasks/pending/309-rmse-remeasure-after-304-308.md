# Task 309: RMSE Re-Measure After Tasks 304–308

## Description
After tasks 304–308 are merged, take a fresh screenshot and measure the RMSE against the reference to track progress toward the 15% target.

Tasks completed:
- 304: Output level meter scale strip added
- 305: GR meter narrowed to thin bar
- 306: Loudness panel large LUFS readout font increased
- 307: Knob face neutral gray, arc more subtle
- 308: Waveform dB label alpha reduced, right-aligned

Measure full-image RMSE and the same sub-regions as task 299:
- Full image (900×500)
- Left level meters (30×378 at x=0, y=30)
- Waveform area (600×400 at x=150, y=50)
- Right panel (100×400 at x=800, y=50)
- Control strip (900×60 at x=0, y=440)

## Produces
None

## Consumes
artifact:M-LIM/build/MLIM_artefacts/Release/Standalone/M-LIM

## Relevant Files
Read: `Scripts/ui-test-helper.sh` — start_app, screenshot, compare_to_reference helpers
Read: `screenshots/task-299-rmse-results.txt` — baseline measurements to compare against

## Acceptance Criteria
- [ ] Run: `source Scripts/ui-test-helper.sh && start_app && screenshot screenshots/task-309-raw.png && stop_app` → Expected: screenshot captured
- [ ] Run: crop and resize commands identical to task 299 methodology → Expected: 900×500 plugin crop saved
- [ ] Run: `compare -metric RMSE screenshots/task-309-mlim-cropped.png screenshots/task-309-ref-cropped.png null: 2>&1` → Expected: full-image RMSE reported
- [ ] Run: sub-region RMSE commands for all 4 regions → Expected: right panel RMSE lower than 0.2960 (task-299 baseline); full image lower than 0.2419

## Tests
None

## Technical Details
Use the exact same crop methodology as task 299:
- M-LIM crop: `convert task-309-raw.png -crop 908x500+509+325 -resize 900x500 task-309-mlim-cropped.png`
- Reference crop: same as task-299 (prol2-main-ui.jpg -crop 1712x1073+97+32 -resize 900x500)
- Sub-region comparisons using `compare -metric RMSE` with geometry flags

Save results to `screenshots/task-309-rmse-results.txt` with the same format as `screenshots/task-299-rmse-results.txt`.

## Dependencies
Requires tasks 304, 305, 306, 307, 308
