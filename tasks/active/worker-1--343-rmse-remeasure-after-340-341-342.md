# Task 343: RMSE Remeasure After Tasks 340, 341, 342

## Description
Rebuild and remeasure RMSE after the three regression-fix tasks:
- Task 340 (done): Revert task-333 regression — re-hide inputMeter_ (left meters should recover from 32.70% → ~23.71%)
- Task 341 (done): Revert waveform gradient to task-317 values (waveform should recover from 23.57% → ~20%)
- Task 342 (done): Revert loudness panel background to #2B2729 (right panel should recover from 31.07% → ~29.66%)

**Expected outcome**: Full-image RMSE should return to ~21-22% range (near task-317's 21.97% best).

## Produces
None

## Consumes
None

## Relevant Files
Read: `Scripts/ui-test-helper.sh` — use for build + screenshot + compare workflow
Read: `screenshots/task-339-rmse-results.txt` — current baseline to compare against

## Acceptance Criteria
- [ ] Run: `cmake --build M-LIM/build -j$(nproc) 2>&1 | grep -c "error:"` → Expected: `0`
- [ ] Run: full RMSE measurement workflow → Expected: full-image RMSE < 23.0%
- [ ] Run: waveform sub-region RMSE → Expected: < 22.0%
- [ ] Run: left meter sub-region RMSE → Expected: < 25.0% (recovering from 32.70%)
- [ ] Run: right panel sub-region RMSE → Expected: < 31.0%
- [ ] Save results to `screenshots/task-343-rmse-results.txt`

## Tests
None

## Technical Details
Use the verified task-299/339 methodology (CRITICAL — do not deviate):

```bash
source Scripts/ui-test-helper.sh
start_app
# Raw screenshot (full 1920x1080)
scrot /workspace/screenshots/task-343-raw.png
stop_app

# Crop plugin area: 908x500 offset 509,325 → resize to 900x500
convert /workspace/screenshots/task-343-raw.png \
  -crop 908x500+509+325 -resize 900x500\! \
  /workspace/screenshots/task-343-mlim-cropped.png

# Reference crop (verified methodology):
convert /reference-docs/reference-screenshots/prol2-main-ui.jpg \
  -crop 1712x1073+97+32 -resize 900x500\! \
  /workspace/screenshots/task-343-ref-cropped.png

# Full-image RMSE:
compare -metric RMSE \
  /workspace/screenshots/task-343-mlim-cropped.png \
  /workspace/screenshots/task-343-ref-cropped.png \
  /workspace/screenshots/task-343-diff-full.png 2>&1

# Sub-region RMSEs:
# Left level meters (30x378 at x=0, y=30):
convert /workspace/screenshots/task-343-mlim-cropped.png -crop 30x378+0+30 /tmp/t343-mlim-left.png
convert /workspace/screenshots/task-343-ref-cropped.png  -crop 30x378+0+30 /tmp/t343-ref-left.png
compare -metric RMSE /tmp/t343-mlim-left.png /tmp/t343-ref-left.png null: 2>&1

# Waveform area (600x400 at x=150, y=50):
convert /workspace/screenshots/task-343-mlim-cropped.png -crop 600x400+150+50 /tmp/t343-mlim-wave.png
convert /workspace/screenshots/task-343-ref-cropped.png  -crop 600x400+150+50 /tmp/t343-ref-wave.png
compare -metric RMSE /tmp/t343-mlim-wave.png /tmp/t343-ref-wave.png null: 2>&1

# Right panel (100x400 at x=800, y=50):
convert /workspace/screenshots/task-343-mlim-cropped.png -crop 100x400+800+50 /tmp/t343-mlim-right.png
convert /workspace/screenshots/task-343-ref-cropped.png  -crop 100x400+800+50 /tmp/t343-ref-right.png
compare -metric RMSE /tmp/t343-mlim-right.png /tmp/t343-ref-right.png null: 2>&1

# Control strip (900x60 at x=0, y=440):
convert /workspace/screenshots/task-343-mlim-cropped.png -crop 900x60+0+440 /tmp/t343-mlim-ctrl.png
convert /workspace/screenshots/task-343-ref-cropped.png  -crop 900x60+0+440 /tmp/t343-ref-ctrl.png
compare -metric RMSE /tmp/t343-mlim-ctrl.png /tmp/t343-ref-ctrl.png null: 2>&1
```

Save comparison images and results to `screenshots/` directory.
Save summary to `screenshots/task-343-rmse-results.txt`.

Methodology note: Always use `scrot` directly (not the `screenshot` helper which may use a
different crop) and use the EXACT crop/resize commands above. Verify methodology by checking
that the task-299 reference measurement reproduces at ~24.19% with the same commands.

## Dependencies
Requires tasks 340, 341, 342
