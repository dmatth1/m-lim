# Task 345: RMSE Remeasure After Task 341

## Description
Rebuild and remeasure RMSE after:
- Task 341: Darken waveform gradient bottom (`displayGradientBottom` `#687090` → `#4A4F6B`)

Check task-343 results (`screenshots/task-343-rmse-results.txt`) for the post-reversion
baseline before reporting improvement or regression.

## Produces
None

## Consumes
None

## Relevant Files
Read: `Scripts/ui-test-helper.sh` — use for build + screenshot + compare workflow
Read: `screenshots/task-343-rmse-results.txt` — baseline to compare against
Read: `src/ui/Colours.h` — verify `displayGradientBottom` is `0xff4A4F6B`

## Acceptance Criteria
- [ ] Run: `cmake --build M-LIM/build -j$(nproc) 2>&1 | grep -c "error:"` → Expected: `0`
- [ ] Run: full RMSE measurement workflow → Expected: full-image RMSE ≤ task-343 full-image result
- [ ] Run: waveform sub-region RMSE → Expected: ≤ task-343 waveform body result
- [ ] Save results to `screenshots/task-345-rmse-results.txt`

## Tests
None

## Technical Details
Use the verified task-299/339/343 methodology (CRITICAL — do not deviate):

```bash
source Scripts/ui-test-helper.sh
start_app
scrot /workspace/screenshots/task-345-raw.png
stop_app

# Crop plugin area: 908x500 offset 509,325 → resize to 900x500
convert /workspace/screenshots/task-345-raw.png \
  -crop 908x500+509+325 -resize 900x500\! \
  /workspace/screenshots/task-345-mlim-cropped.png

# Reference crop (verified methodology):
convert /reference-docs/reference-screenshots/prol2-main-ui.jpg \
  -crop 1712x1073+97+32 -resize 900x500\! \
  /workspace/screenshots/task-345-ref-cropped.png

# Full-image RMSE:
compare -metric RMSE \
  /workspace/screenshots/task-345-mlim-cropped.png \
  /workspace/screenshots/task-345-ref-cropped.png \
  /workspace/screenshots/task-345-diff-full.png 2>&1

# Waveform area (600x400 at x=150, y=50):
convert /workspace/screenshots/task-345-mlim-cropped.png -crop 600x400+150+50 /tmp/t345-mlim-wave.png
convert /workspace/screenshots/task-345-ref-cropped.png  -crop 600x400+150+50 /tmp/t345-ref-wave.png
compare -metric RMSE /tmp/t345-mlim-wave.png /tmp/t345-ref-wave.png null: 2>&1

# Left level meters (30x378 at x=0, y=30):
convert /workspace/screenshots/task-345-mlim-cropped.png -crop 30x378+0+30 /tmp/t345-mlim-left.png
convert /workspace/screenshots/task-345-ref-cropped.png  -crop 30x378+0+30 /tmp/t345-ref-left.png
compare -metric RMSE /tmp/t345-mlim-left.png /tmp/t345-ref-left.png null: 2>&1

# Right panel (100x400 at x=800, y=50):
convert /workspace/screenshots/task-345-mlim-cropped.png -crop 100x400+800+50 /tmp/t345-mlim-right.png
convert /workspace/screenshots/task-345-ref-cropped.png  -crop 100x400+800+50 /tmp/t345-ref-right.png
compare -metric RMSE /tmp/t345-mlim-right.png /tmp/t345-ref-right.png null: 2>&1

# Control strip (900x60 at x=0, y=440):
convert /workspace/screenshots/task-345-mlim-cropped.png -crop 900x60+0+440 /tmp/t345-mlim-ctrl.png
convert /workspace/screenshots/task-345-ref-cropped.png  -crop 900x60+0+440 /tmp/t345-ref-ctrl.png
compare -metric RMSE /tmp/t345-mlim-ctrl.png /tmp/t345-ref-ctrl.png null: 2>&1
```

Save all comparison images and results to `screenshots/` directory.
Save summary to `screenshots/task-345-rmse-results.txt`.

## Dependencies
Requires task 341
