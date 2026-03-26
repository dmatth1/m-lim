# Task 339: RMSE Remeasure After Visual Parity Batch (Tasks 330, 333, 336–338)

## Description
Rebuild and remeasure RMSE after the current batch of visual parity improvements:
- Task 328 (done): Revert control strip knob height regression (kKnobRowH=56, kControlStripH=92)
- Task 329 (done): Revert output meter width regression (kOutputMeterW=58)
- Task 331 (done): Darken loudness panel background (#1E1C21)
- Task 330 (active): Fix waveform gradient colors (#8992AB top, #5C6880 bottom)
- Task 333 (active): Show input level meter (30px strip on left edge)
- Task 336: Waveform gradient fine-tune (if needed after task 330)
- Task 337: Loudness panel histogram taller
- Task 338: GR meter background matches waveform gradient

**Expected improvements over task 324 baseline (23.14% full-image RMSE):**
- Control strip: ~22.57% (restored from 328/329 reverts)
- Waveform: ~19–21% (gradient fix + input meter restore + GR meter blend)
- Right panel: ~28–29% (darker background done, histogram taller)
- Full image: < 22%

## Produces
None

## Consumes
None

## Relevant Files
Read: `Scripts/ui-test-helper.sh` — use for build + screenshot + compare workflow
Read: `screenshots/task-324-rmse-results.txt` — baseline to compare against (if exists)

## Acceptance Criteria
- [ ] Run: `cd /workspace/M-LIM && cmake --build build -j$(nproc) 2>&1 | grep -c "error:"` → Expected: `0`
- [ ] Run: full RMSE measurement workflow → Expected: full-image RMSE < 22.0%
- [ ] Run: waveform sub-region RMSE → Expected: < 21.0%
- [ ] Run: control strip sub-region RMSE → Expected: ≤ 22.6%
- [ ] Save results to `screenshots/task-339-rmse-results.txt`

## Tests
None

## Technical Details
Use the standard measurement methodology from previous RMSE tasks:

```bash
source Scripts/ui-test-helper.sh
start_app
screenshot "screenshots/task-339-raw.png"
stop_app

# Crop plugin area to 900x500, compare to reference
# See Scripts/ui-test-helper.sh for compare_to_reference function
```

Sub-region measurements:
- Left level meters: 30x378 at x=0, y=30
- Waveform area: 600x400 at x=150, y=50
- Right panel: 100x400 at x=800, y=50
- Control strip: 900x60 at x=0, y=440

Save all images and results to `screenshots/` directory.

## Dependencies
Requires tasks 330, 333, 337, 338 (and optionally 336)
