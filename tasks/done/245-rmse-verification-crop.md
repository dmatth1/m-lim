# Task 245: RMSE Verification — Crop and Compare Plugin Window Only

## Description
The current RMSE measurement in task 238 compares a full 1280×800 screenshot (plugin
window surrounded by black desktop) against the 1900×1184 reference JPEG. Both images
are scaled to 900×500, meaning the large black border in M-LIM's screenshot dilutes the
comparison and inflates RMSE.

For an accurate measurement, the comparison must use **only the plugin window content**
(cropped to the actual plugin bounds, without desktop background), compared against the
Pro-L 2 reference after similar cropping to the plugin area.

**Steps**:
1. Launch the standalone in Xvfb.
2. Use `xdotool` to get the window geometry (position + size).
3. Crop the screenshot to the plugin bounds using `convert -crop`.
4. Resize to 900×500 for comparison.
5. Crop the reference JPEG to the Pro-L 2 plugin area (which sits within the 1900×1184
   image). The plugin area in `prol2-main-ui.jpg` starts at approximately:
   x=97, y=32, width=1712, height=1073 (verify with pixel sampling for the dark
   plugin border).
6. Run `compare -metric RMSE` on the two cropped+resized images.
7. Record the RMSE value in `screenshots/task-245-rmse-results.txt`.

The target RMSE is ≤ 0.15 (15%). If current value is above target, document the largest
remaining visual differences as findings in the results file.

## Produces
None

## Consumes
None

## Relevant Files
Read:   `Scripts/ui-test-helper.sh` — start_app / screenshot / compare helpers
Read:   `/reference-docs/reference-screenshots/prol2-main-ui.jpg` — reference image
Create: `screenshots/task-245-rmse-results.txt` — RMSE measurement results

## Acceptance Criteria
- [ ] Run: `cat screenshots/task-245-rmse-results.txt` → Expected: file exists with RMSE value and top-3 visual difference findings
- [ ] Run: `cd build && ctest --output-on-failure` → Expected: all tests pass

## Tests
None

## Technical Details
Reference image crop: `prol2-main-ui.jpg` is 1900×1184. The plugin window appears to
start near x=97, y=32. Use:
```bash
convert /reference-docs/reference-screenshots/prol2-main-ui.jpg \
    -crop 1712x1073+97+32 +repage \
    -resize 900x500! /tmp/ref-plugin.png
```
Verify crop bounds by sampling border pixels (should be dark gray #252525 at edges).

For M-LIM, use xdotool or crop by known offsets from the screenshot:
```bash
# Plugin appears at ~193,131 in 1280x800 screenshot
convert /workspace/screenshots/audit-after-colours.png \
    -crop 896x550+193+131 +repage \
    -resize 900x500! /tmp/mlim-plugin.png
compare -metric RMSE /tmp/mlim-plugin.png /tmp/ref-plugin.png /tmp/diff-cropped.png 2>&1
```

## Dependencies
Requires tasks 239, 240, 241, 242, 243, 244 to be completed first for accurate final measurement.
