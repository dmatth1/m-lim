# Task: Waveform Midzone Tent — Slight Alpha Reduction

## Description

Pixel analysis at y=250 (56% height, the mid-zone tent peak) shows M-LIM is slightly
over-bright compared to the reference:

- M-LIM at y=250, x=300: srgb(126, 134, 162)
- Reference at y=250, x=300: srgb(121, 128, 155)
- Delta: M-LIM is +5R, +6G, +7B too bright

The two overlapping tent gradients (mid tent at 0.80 peak and center tent at 0.65 peak)
both peak at exactly 58% height. Their combined effect over-boosts the midzone slightly.

**Fix**: Reduce both tent peak alphas by approximately 15%:
- Mid tent: 0.80 → 0.70
- Center tent: 0.65 → 0.55

Mathematical projection:
- After reduction, y=250 composited result ≈ srgb(122, 131, 159) — within 2-3 units of reference

This is a conservative reduction that should improve the mid-zone match without significantly
affecting surrounding regions.

## Relevant Files

Modify: `src/ui/WaveformDisplay.cpp` — lines 315-354 (two tent gradient passes)
Read: `screenshots/task-397-rmse-results.txt` — wave-21 baseline

## Acceptance Criteria

- [ ] Run RMSE on wave region → Expected: wave RMSE ≤ 16.72% (wave-21 baseline)
- [ ] Run RMSE on full image → Expected: full RMSE ≤ 19.46%
- [ ] Save screenshot and RMSE to `screenshots/task-NNN-after.png` and `screenshots/task-NNN-rmse-results.txt`

## Tests
None

## Technical Details

**Current code** (WaveformDisplay.cpp lines 307–355):
```cpp
// Mid tent: peak alpha 0.80, covers 36%–82% centred at 58%
midFill.withAlpha(0.80f)   // both rise and fall

// Center tent: peak alpha 0.65, covers 40%–76% centred at 58%
cCol.withAlpha(0.65f)      // both rise and fall
```

**Change to**:
```cpp
// Mid tent: peak alpha 0.70 (was 0.80)
midFill.withAlpha(0.70f)

// Center tent: peak alpha 0.55 (was 0.65)
cCol.withAlpha(0.55f)
```

Also try: 0.72/0.58, 0.68/0.52 if 0.70/0.55 doesn't give best RMSE.

**RMSE methodology**:
```bash
# Build, launch, screenshot, crop, measure (see task-397 for full commands)
# Key measurements:
convert /tmp/mlim.png -crop 640x500+0+0 +repage /tmp/mlim-wave.png
convert /tmp/ref.png  -crop 640x500+0+0 +repage /tmp/ref-wave.png
compare -metric RMSE /tmp/mlim-wave.png /tmp/ref-wave.png /dev/null 2>&1
```

**Note**: If this is being done together with the upper idle fill new pass task, test BOTH
changes together and independently to confirm combined RMSE is better.

## Dependencies
None (can run in parallel with upper idle fill task)
