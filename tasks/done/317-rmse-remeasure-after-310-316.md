# Task 317: RMSE Remeasure After Visual Tasks 306–316

## Description
Measure the RMSE improvement after all visual parity tasks from batch 306–316 are merged.
This provides a quantitative checkpoint before the next audit cycle.

**Baseline (task 303):**
- Full image: 24.19%
- Left level meters (30×378 at x=0, y=30): 26.00%
- Waveform area (600×400 at x=150, y=50): 23.03%
- Right panel (100×400 at x=800, y=50): 29.60%
- Control strip (900×60 at x=0, y=440): 22.56%

**Tasks covered by this remeasure:**
- Done 305: waveform gradient neutral top / blue bottom
- Active 306: loudness panel large LUFS readout font
- Active 307: knob face neutral gray, arc subtle
- Active 309: rotary knob face lighter gray (overlaps 307)
- Active wdb306: waveform dB label opacity reduced
- Active wlm307: level meter safe-zone color muted blue-gray
- 310: GR meter warm color gradient
- 311: output meter dB scale strip
- 312: GR meter narrowed to thin bar
- 316: waveform dB labels right-aligned (depends on active wdb306)

**Target:** Full image < 22.0%

## Produces
None

## Consumes
artifact:M-LIM/build/MLIM_artefacts/Release/Standalone/M-LIM

## Relevant Files
Read: `M-LIM/Scripts/ui-test-helper.sh` — start_app, screenshot, compare helpers
Read: `screenshots/task-303-rmse-results.txt` — baseline measurements

## Acceptance Criteria
- [ ] Run: `cmake --build /workspace/M-LIM/build --config Release -j$(nproc) --target MLIM_Standalone 2>&1 | tail -3` → Expected: exit 0
- [ ] Run: `source Scripts/ui-test-helper.sh && start_app && screenshot screenshots/task-317-raw.png && stop_app` → Expected: screenshot captured
- [ ] Run full RMSE comparison → Expected: full image RMSE < 0.230 (improvement from 0.2419 baseline)
- [ ] Save results to `screenshots/task-317-rmse-results.txt`

## Tests
None

## Technical Details
Use the same crop methodology as task 303/299:
```bash
# Crop plugin area (908x500 at 509,325)
convert screenshots/task-317-raw.png -crop 908x500+509+325 +repage \
    -resize 900x500! screenshots/task-317-mlim-cropped.png

# Crop reference
convert /reference-docs/reference-screenshots/prol2-main-ui.jpg \
    -crop 1712x1073+97+32 +repage -resize 900x500! screenshots/task-317-ref-cropped.png

# Full image RMSE
compare -metric RMSE screenshots/task-317-ref-cropped.png \
    screenshots/task-317-mlim-cropped.png screenshots/task-317-diff-full.png 2>&1

# Sub-region RMSE
# Left meters: 30x378 at x=0, y=30
# Waveform: 600x400 at x=150, y=50
# Right panel: 100x400 at x=800, y=50
# Control strip: 900x60 at x=0, y=440
```

Save results to `screenshots/task-317-rmse-results.txt` with per-region breakdown and delta from task-303 baseline.

## Dependencies
Requires tasks 310, 311, 312, 316
