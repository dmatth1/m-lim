# Task 332: RMSE Remeasure After Tasks 328–331

## Description
Rebuild and remeasure RMSE after visual parity improvements from tasks 328–331:
- 328: Revert control strip knob height regression (kKnobRowH 72→56, kControlStripH 108→92)
- 329: Revert output meter width regression (kOutputMeterW 80→58)
- 330: Fix waveform gradient colors (#686468→#8992AB top, #506090→#5C6880 bottom)
- 331: Darken loudness panel background (#2B2729→#1E1C21)

**Expected improvements:**
- Control strip: 23.40% → ~22.57% (revert of 318)
- Waveform: 22.08% → ~20.00% or below (revert of 318+320, gradient fix)
- Right panel: 29.66% → ~29% (darker panel background)
- Full image: 23.14% → ~21.5% or better

## Produces
None

## Consumes
None

## Relevant Files
Read: `Scripts/ui-test-helper.sh` — use for build + screenshot + compare workflow
Read: `screenshots/task-324-rmse-results.txt` — baseline to compare against

## Acceptance Criteria
- [ ] Run: `cd /workspace/M-LIM && cmake --build build -j$(nproc) 2>&1 | grep -c "error:"` → Expected: `0`
- [ ] Run: full RMSE measurement workflow → Expected: full-image RMSE < 22.0%
- [ ] Run: waveform sub-region RMSE → Expected: < 21.0%
- [ ] Run: control strip sub-region RMSE → Expected: ≤ 22.6%
- [ ] Save results to `screenshots/task-332-rmse-results.txt`

## Tests
None

## Technical Details
Use the standard measurement methodology from task 324:

```bash
source Scripts/ui-test-helper.sh
start_app

# Crop plugin area: 908x500 at +509+325 → resize to 900x500
# Reference crop: 1712x1073+97+32 → resize 900x500
# compare -metric RMSE

screenshot "task-332-raw.png"
# ... standard crop + compare pipeline
```

Sub-region measurements:
- Left level meters: 30x378 at x=0, y=30
- Waveform area: 600x400 at x=150, y=50
- Right panel: 100x400 at x=800, y=50
- Control strip: 900x60 at x=0, y=440

Save all images and results to `screenshots/` directory.

## Dependencies
Requires tasks 328, 329, 330, 331
