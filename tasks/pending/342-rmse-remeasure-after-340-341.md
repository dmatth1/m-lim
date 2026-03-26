# Task 342: RMSE Remeasure After Tasks 340–341

## Description
Rebuild and remeasure RMSE after:
- Task 340: Re-hide input level meter (revert task-333 regression)
- Task 341: Darken waveform gradient bottom (`#687090` → `#4A4F6B`)

**Current baseline (VisualParityAuditor, 2026-03-26 with precise crop 900x500+514+346):**
- Full image: 25.73%
- Left sub-region (30x378 at x=0,y=30): 26.74%
- Waveform body (600x400 at x=150,y=50): 23.76%
- Right panel (100x400 at x=800,y=50): 22.88%
- Control strip (900x60 at x=0,y=440): 18.31%

**Expected after tasks 340+341:**
- Full image: < 24%
- Left sub-region: < 22% (input meter hidden → waveform gradient at left edge)
- Waveform body: < 22% (darker gradient bottom)

## Produces
None

## Consumes
None

## Relevant Files
Read: `Scripts/ui-test-helper.sh` — use if available, otherwise follow manual steps below

## Acceptance Criteria
- [ ] Run: `cd /workspace/M-LIM && cmake --build build -j$(nproc) 2>&1 | grep -c "error:"` → Expected: `0`
- [ ] Run: full RMSE measurement → Expected: full-image RMSE < 24.0%
- [ ] Run: left sub-region RMSE (30x378 at x=0,y=30) → Expected: < 22.0%
- [ ] Run: waveform sub-region RMSE (600x400 at x=150,y=50) → Expected: < 22.5%
- [ ] Save results to `screenshots/task-342-rmse-results.txt`

## Tests
None

## Technical Details
Use this measurement methodology (consistent with prior tasks):

```bash
# Start Xvfb at 1920x1080
sudo Xvfb :98 -screen 0 1920x1080x24 -nolisten tcp &
sleep 2
DISPLAY=:98 /workspace/M-LIM/build/MLIM_artefacts/Release/Standalone/M-LIM &
sleep 5
DISPLAY=:98 scrot /tmp/task342-raw.png
pkill -f "M-LIM"; pkill Xvfb

# Find plugin position (scan for waveform color ~#8891AA)
# Then crop precisely to 900x500 matching plugin content area
# Typical crop offset in this environment: 900x500+514+346

# Compute RMSE against reference
convert /reference-docs/reference-screenshots/prol2-main-ui.jpg -resize 900x500! /tmp/ref-900x500.png
compare -metric RMSE /tmp/ref-900x500.png /tmp/task342-crop.png /dev/null 2>&1

# Sub-region breakdowns
compare -metric RMSE \
    <(convert /tmp/ref-900x500.png -crop 30x378+0+30 +repage png:-) \
    <(convert /tmp/task342-crop.png -crop 30x378+0+30 +repage png:-) \
    /dev/null 2>&1  # left sub-region

compare -metric RMSE \
    <(convert /tmp/ref-900x500.png -crop 600x400+150+50 +repage png:-) \
    <(convert /tmp/task342-crop.png -crop 600x400+150+50 +repage png:-) \
    /dev/null 2>&1  # waveform sub-region

compare -metric RMSE \
    <(convert /tmp/ref-900x500.png -crop 100x400+800+50 +repage png:-) \
    <(convert /tmp/task342-crop.png -crop 100x400+800+50 +repage png:-) \
    /dev/null 2>&1  # right panel sub-region

compare -metric RMSE \
    <(convert /tmp/ref-900x500.png -crop 900x60+0+440 +repage png:-) \
    <(convert /tmp/task342-crop.png -crop 900x60+0+440 +repage png:-) \
    /dev/null 2>&1  # control strip sub-region
```

Save all measurements to `screenshots/task-342-rmse-results.txt`.

## Dependencies
Requires tasks 340, 341
