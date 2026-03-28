# Task: RMSE Remeasure After Sweep 42 Visual Fixes

## Description
After completing the visual parity tasks from sweep 42 (output meter idle alpha, histogram idle alpha boost, loudness panel darken, input meter idle alpha, waveform label contrast, GR meter width), capture a fresh RMSE measurement and record the new baseline.

## Produces
None

## Consumes
None

## Relevant Files
Read: `Scripts/ui-test-helper.sh` — screenshot capture helpers

## Acceptance Criteria
- [ ] Run: Build standalone, launch headlessly, capture screenshot, crop, compare against reference → Expected: RMSE recorded
- [ ] Run: `cat screenshots/wave26-rmse-results.txt` → Expected: Full and subregion RMSE values present, Full < 17%

## Tests
None

## Technical Details
RMSE methodology:
- Reference crop: `prol2-main-ui.jpg -crop 1712x1073+97+32 +repage -resize 900x500!`
- M-LIM: scrot 1920x1080, crop `908x500+509+325`, resize `900x500!`
- Subregions: Wave (640x500+0+0), Left (80x500+640+0), Right (180x500+720+0), Control (900x90+0+410)
- Previous baseline: Full=18.57%, Wave=16.27%, Left=24.49%, Right=22.71%, Control=19.28%

Save results to `screenshots/wave26-rmse-results.txt`.

## Dependencies
Requires all visual parity tasks from sweep 42 to be complete
