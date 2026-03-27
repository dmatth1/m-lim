# Task: Output Meter Idle Gradient Brighten

## Description
The output meter warm zone (x=800-870 in the 900x500 comparison crop) is significantly darker than the reference. Pixel analysis shows:
- Reference average: `#70665A` (R=112, G=102, B=90)
- M-LIM current: `#564B39` (R=86, G=75, B=57)
- Gap: M-LIM is ~26-33 units darker across all channels

The output meter idle gradient needs to be brightened so the warm amber tone better matches the reference Pro-L 2 output meter appearance. The fix is to increase the alpha of the warm idle gradient in `LevelMeter::drawChannel` (the `idleGrad`) or to brighten the colors involved.

Specifically in `LevelMeter.cpp`, the idle structural gradient starts with `MLIMColours::meterDanger.withAlpha(0.10f)` at top and `MLIMColours::meterSafe.darker(0.3f).withAlpha(0.80f)` at bottom. The warm amber zone (between dangerBot and warnBot) uses `grMeterMid.withAlpha(0.28f)` and `meterWarning.withAlpha(0.25f)`. Increasing the warm zone alphas and/or lightening the bottom gradient color should close the 26-33 unit brightness gap.

## Produces
None

## Consumes
None

## Relevant Files
Modify: `src/ui/LevelMeter.cpp` — increase idle gradient warm zone alphas (lines ~90-120)
Read: `src/ui/Colours.h` — color constants referenced in the idle gradient

## Acceptance Criteria
- [ ] Run: `source Scripts/ui-test-helper.sh && start_app && screenshot screenshots/task-output-meter-after.png && compare_to_reference /reference-docs/reference-screenshots/prol2-main-ui.jpg screenshots/task-output-meter-after.png 0.15 && stop_app` → Expected: Right zone RMSE improves from 23.21% toward 22% or below
- [ ] Run: pixel color check `convert screenshots/task-output-meter-after.png -crop 70x280+800+100 +repage -resize 1x1! txt:-` → Expected: R channel ≥ 95 (up from 86)

## Technical Details
In `LevelMeter::drawChannel`, the idle gradient block builds `idleGrad` with:
- Bottom color: `MLIMColours::meterSafe.darker(0.3f).withAlpha(0.80f)` — try `.withAlpha(0.95f)`
- Warm mid color at dangerBot: `grMeterMid.withAlpha(0.28f)` — try `0.48f`
- Warm yellow at warnBot: `meterWarning.withAlpha(0.25f)` — try `0.40f`
- Extended warm at -12 dB: `grMeterLow.withAlpha(0.20f)` — try `0.35f`

Adjust these alphas incrementally to raise the output meter composite average from #564B39 toward the target #70665A. Rebuild only the Standalone target: `cmake --build build --target MLIM_Standalone -j$(nproc)`.

## Dependencies
None
