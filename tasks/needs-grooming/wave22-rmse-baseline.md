# Task: Wave 22 RMSE Baseline Measurement

## Description
Record the official RMSE baseline at the start of wave 22. The build fix (ALSA linking in
CMakeLists.txt) has been applied. Capture a fresh screenshot, compute full regional RMSE,
and commit as the wave 22 reference point.

Build fix applied: moved `$<$<BOOL:${ALSA_FOUND}>:${ALSA_LIBRARIES}>` from PRIVATE to PUBLIC
in the MLIM target_link_libraries so the Standalone binary correctly links libasound.

## Produces
None

## Consumes
None

## Relevant Files
Read: `screenshots/` — existing screenshots for reference
Modify: `screenshots/task-NNN-rmse-results.txt` — create baseline file

## Acceptance Criteria
- [ ] Run: `cmake --build build --target MLIM_Standalone -j$(nproc)` → Expected: exit 0
- [ ] Run: launch app + screenshot + RMSE → Expected: Full RMSE recorded; all 5 regions measured
- [ ] Run: `git log --oneline -3` → Expected: baseline commit visible

## Tests
None

## Technical Details
Use RMSE methodology from CLAUDE.md:
- Reference crop: prol2-main-ui.jpg -crop 1712x1073+97+32 +repage -resize 900x500!
- M-LIM crop: scrot 1920x1080, crop 908x500+509+325, resize 900x500!
- Regions: Full (full 900x500), Wave (900x300+0+0), Left (90x500+0+0),
           Right (225x500+675+0), Control (900x100+0+400)
- Wave 21 baseline (task-397): Full=19.46%, Wave=16.72%, Left=28.71%, Right=23.09%, Control=21.02%

Note: The wave22 audit measured slightly different values (Full=19.46%, Wave=19.31%, Right=23.95%)
due to fresh build and different crop methodology. Worker should establish definitive baseline
using the canonical CLAUDE.md methodology.

## Dependencies
None
