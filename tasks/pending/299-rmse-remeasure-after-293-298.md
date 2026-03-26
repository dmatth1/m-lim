# Task 299: RMSE Re-Measure After All Pending Visual Tasks

## Description
Re-measure the RMSE between M-LIM and the Pro-L 2 reference after the following tasks:
- Task 293: ADVANCED button returned to control strip (DONE)
- Task 294: Remove input meter from left layout
- Task 296: Level meter right panel visual match (active)
- Task 297: Level meter inactive background darkened
- Task 298: Waveform dB labels contrast increased
- Task 300: Algorithm selector compact single slot
- Task 301: TopBar height reduced (30 → 24 px)
- Task 302: Waveform background gradient further darkened

Report full-image RMSE and sub-region breakdowns.

## Produces
None

## Consumes
artifact:build/MLIM_artefacts/Release/Standalone/M-LIM

## Relevant Files
Read: `Scripts/ui-test-helper.sh` — for `start_app`, `screenshot`, `compare_to_reference`
Read: `/workspace/screenshots/task-288-rmse-results.txt` — baseline for comparison
Read: `/reference-docs/reference-screenshots/prol2-main-ui.jpg` — reference image

## Acceptance Criteria
- [ ] Run: `source /workspace/Scripts/ui-test-helper.sh && start_app && screenshot "task-299-after.png" && stop_app` → Expected: screenshot saved
- [ ] Run: RMSE comparison → Expected: full-image RMSE < 0.245 (improvement from 0.2570)
- [ ] Run: Left level meter region RMSE → Expected: < 0.300 (improvement from 0.3336 regression)
- [ ] Results saved to `screenshots/task-299-rmse-results.txt`

## Tests
None

## Technical Details
Use the same methodology as task 288:
```bash
source /workspace/Scripts/ui-test-helper.sh
cd /workspace
start_app
sleep 3
DISPLAY=:99 import -window root /workspace/screenshots/task-299-raw.png
stop_app

# Crop plugin area (get bounds from xdotool)
convert screenshots/task-299-raw.png -crop 908x500+509+325 -resize 900x500! +repage screenshots/task-299-mlim-cropped.png

# Crop reference to same scale
convert /reference-docs/reference-screenshots/prol2-main-ui.jpg \
  -crop 1712x1073+97+32 -resize 900x500! +repage screenshots/task-299-ref-cropped.png

# Full image RMSE
compare -metric RMSE screenshots/task-299-ref-cropped.png screenshots/task-299-mlim-cropped.png \
  screenshots/task-299-diff-full.png 2>&1

# Sub-regions
# Left meter (30x378 at x=0, y=30)
convert screenshots/task-299-mlim-cropped.png -crop 30x378+0+30 screenshots/task-299-left-meter.png
compare -metric RMSE <(convert screenshots/task-299-ref-cropped.png -crop 30x378+0+30 png:-) \
  screenshots/task-299-left-meter.png /tmp/left-diff.png 2>&1

# Waveform center
# Control strip
# Right panel
```

Save results summary to `screenshots/task-299-rmse-results.txt`.

## Dependencies
Requires tasks 294, 296, 297, 298, 300, 301, 302
