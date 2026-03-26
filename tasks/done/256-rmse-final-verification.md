# Task 256: Final RMSE Verification After LookAndFeel Fix

## Description
Measure the final RMSE after task 255 (LookAndFeel toggle fix) is complete.
Document remaining visual differences and whether the ≤15% RMSE target is achievable
from a static idle screenshot, or whether a methodology adjustment is needed.

**Steps**:
1. Build latest code.
2. Launch standalone on Xvfb :99 (1920×1080).
3. Use `xdotool` to get window position.
4. Crop the plugin editor area (exclude standalone chrome: title bar + audio-muted banner).
5. Resize M-LIM crop to 900×500.
6. Crop reference `prol2-main-ui.jpg` to plugin area (`-crop 1712x1073+97+32`, resize to 900×500).
7. Run `compare -metric RMSE`.
8. If RMSE > 15%, compute RMSE for the **bottom control strip only** (crop `0,370,900,130`
   from both 900×500 images) to isolate the structural UI from the empty waveform area.
9. Document findings in `screenshots/task-256-rmse-results.txt`.

## Produces
None

## Consumes
None

## Relevant Files
Read:   `Scripts/ui-test-helper.sh` — helper functions
Read:   `/reference-docs/reference-screenshots/prol2-main-ui.jpg` — reference
Create: `screenshots/task-256-rmse-results.txt` — results

## Acceptance Criteria
- [ ] Run: `cat screenshots/task-256-rmse-results.txt` → Expected: file exists with full RMSE and control-strip-only RMSE values
- [ ] Run: `cd /workspace/M-LIM/build && ctest --output-on-failure` → Expected: all tests pass

## Tests
None

## Technical Details
Full comparison command:
```bash
compare -metric RMSE /tmp/mlim-crop.png /tmp/ref-crop.png /tmp/diff.png 2>&1
```

Control strip crop (bottom 26% of image):
```bash
convert /tmp/mlim-crop.png -crop 900x130+0+370 +repage /tmp/mlim-strip.png
convert /tmp/ref-crop.png  -crop 900x130+0+370 +repage /tmp/ref-strip.png
compare -metric RMSE /tmp/mlim-strip.png /tmp/ref-strip.png /tmp/diff-strip.png 2>&1
```

Record both values. If control-strip RMSE ≤ 15%, note that the overall RMSE is dominated
by the empty waveform (expected) and the structural UI matches the target.

## Dependencies
Requires task 255
