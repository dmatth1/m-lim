# Task 357: Reduce Waveform Grid Line Brightness to Match Reference Composite

## Description

The waveform dB grid lines in M-LIM appear too bright compared to the reference. At idle state
(no audio), M-LIM shows grid lines on a light medium-gray background (`#686468`), making them
stand out strongly. In the reference, grid lines appear against a very dark background (~`#262127`)
due to audio fills covering the background — making them more subtle.

**Analysis:**

Grid line composite color approximation:
- M-LIM (idle): `waveformGridLine` at 60% alpha on `#686468` bg → composite ≈ `#8A8FA0` (medium)
- Reference: `waveformGridLine` at 60% alpha on `#262127` bg → composite ≈ `#6A707E` (darker)

The reference grid lines appear DARKER in absolute terms because they're overlaid on dark fills.
M-LIM's lines on a lighter background appear brighter, creating an RMSE discrepancy at each
grid line position.

**Fix:**

Reduce the grid line alpha from `0.6f` to `0.35f` in `WaveformDisplay.cpp` where grid lines are
drawn. This darkens the composite grid line appearance to better approximate the reference:
- New M-LIM grid composite: `waveformGridLine` at 35% alpha on `#686468` → ≈ `#737680` (darker,
  closer to reference `#6A707E`)

Also consider reducing grid line label text alpha from `0.35f` to `0.25f` to match reference
proportionally.

## Produces
None

## Consumes
None

## Relevant Files
Modify: `M-LIM/src/ui/WaveformDisplay.cpp` — change grid line alpha from 0.6f to 0.35f
Read: `M-LIM/src/ui/Colours.h` — waveformGridLine color `0xff9AA0B4`

## Acceptance Criteria
- [ ] Run: `cmake --build M-LIM/build -j$(nproc) 2>&1 | tail -3` → Expected: build succeeds
- [ ] Run: `source Scripts/ui-test-helper.sh && start_app && screenshot_safe "task-357-after.png" && stop_app` → Expected: screenshot captured
- [ ] Visually verify: Grid lines still visible but less prominent than before
- [ ] Run: RMSE comparison → Expected: waveform sub-region RMSE ≤ 20.55% (should improve)

## Tests
None

## Technical Details

```bash
convert /reference-docs/reference-screenshots/prol2-main-ui.jpg \
    -crop 1712x1073+97+32 +repage -resize 900x500! /tmp/ref.png
source Scripts/ui-test-helper.sh && start_app && sleep 2
scrot /tmp/t357-raw.png && stop_app
convert /tmp/t357-raw.png -crop 908x500+509+325 +repage -resize 900x500! /tmp/t357-mlim.png

# Waveform sub-region
compare -metric RMSE \
  <(convert /tmp/ref.png -crop 600x400+150+50 +repage png:-) \
  <(convert /tmp/t357-mlim.png -crop 600x400+150+50 +repage png:-) \
  /dev/null 2>&1
```

Baseline waveform RMSE: 20.55% (task-354)
If RMSE increases by more than 0.15%, revert to 0.6f alpha.

Find the grid line draw call:
```bash
grep -n "withAlpha\|waveformGridLine" /workspace/M-LIM/src/ui/WaveformDisplay.cpp
```

## Dependencies
None
