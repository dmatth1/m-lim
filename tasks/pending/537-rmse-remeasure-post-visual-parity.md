# Task 537: RMSE Remeasure After Visual Parity Tasks 535-536

## Description
After completing visual parity tasks 535 (right panel) and 536 (left meters), capture a fresh RMSE measurement and record the new baseline. This task should be picked up after both 535 and 536 are done.

## Produces
None

## Consumes
None

## Relevant Files
Read: `Scripts/ui-test-helper.sh` — screenshot capture helpers
Read: `screenshots/wave23-rmse-results.txt` — previous baseline (Full=19.47%, Right=26.44%, Left=25.85%)

## Acceptance Criteria
- [ ] Run: `cmake --build build --target MLIM_Standalone_Standalone -j$(nproc)` → Expected: build succeeds
- [ ] Run: Build, launch headlessly, capture screenshot, crop, compare against reference → Expected: RMSE recorded
- [ ] Run: `cat screenshots/wave25-rmse-results.txt` → Expected: Full and subregion RMSE values present

## Tests
None

## Technical Details
RMSE methodology:
- Reference crop: `prol2-main-ui.jpg -crop 1712x1073+97+32 +repage -resize 900x500!`
- M-LIM: scrot 1920x1080, crop `908x500+509+325`, resize `900x500!`
- Subregions: Wave (640x500+0+0), Left (80x500+640+0), Right (180x500+720+0), Control (900x90+0+410)
- Previous baseline: Full=19.47%, Wave=15.90%, Left=25.85%, Right=26.44%, Control=18.97%

Save results to `screenshots/wave25-rmse-results.txt`.

## Dependencies
Requires tasks 535, 536
