# Task 350: Dim Waveform Ceiling Line to Reduce Top-Area Artifact

## Description

The `waveformCeilingLine` color (`0xCCDD4444` = warm red at 80% alpha) draws a very
prominent pink/red line at the top of the WaveformDisplay. In the static screenshot
(no audio), the line at y≈20-22 of the crop (or y≈36-38 after task 349) creates a
bright artifact: ~(197,74,75) per pixel vs reference dark (~47,42,46).

The reference Pro-L 2 uses a white/light gray ceiling line that is visually subtle —
the warm red 80%-alpha line in M-LIM is significantly more saturated/bright than the
reference ceiling indicator.

Fix: Reduce alpha from `0xCC` (80%) to `0x50` (31%). The line remains functional and
visible to users (clearly marks 0 dBFS / ceiling) but its raw pixel brightness is closer
to the reference style.

This is a VISUAL parity fix, not just an RMSE fix. The current bright red is inconsistent
with Pro-L 2's subtle ceiling marker.

## Produces
None

## Consumes
None

## Relevant Files
Modify: `M-LIM/src/ui/Colours.h` — change `waveformCeilingLine` (line 34)
Read: `M-LIM/src/ui/WaveformDisplay.cpp` lines 327-353 — ceiling line draw code (uses 1.5px line + label)
Skip: `M-LIM/src/dsp/` — not needed

## Acceptance Criteria
- [ ] Run: `cmake --build build -j$(nproc) 2>&1 | grep -c "error:"` → Expected: `0`
- [ ] Run: `source Scripts/ui-test-helper.sh && start_app 97 && sleep 4 && screenshot "task-350-after.png" && stop_app` → Expected: screenshot captured
- [ ] Run: ceiling line pixel check at center of waveform top: inspect crop y≈20-22 (or y≈36-38 if task 349 merged). Expected: red channel ≤ 160 (was 197); green channel ≥ 70 (was 74, should be slightly better after dim). Net: visually less saturated.
- [ ] Run: waveform sub-region RMSE check (600x400 at x=150,y=50): `compare -metric RMSE <(convert screenshots/task-350-after.png -crop 900x500+0+0 +repage -crop 600x400+150+50 +repage png:-) <(convert screenshots/audit-ref-crop.png -crop 600x400+150+50 +repage png:-) /dev/null 2>&1` → Expected: no regression from 0.1963 baseline (≤ 0.210)

## Tests
None

## Technical Details

In `M-LIM/src/ui/Colours.h`:
```cpp
// Before:
const juce::Colour waveformCeilingLine { 0xCCDD4444 };  // warm red, ~80% alpha
// After:
const juce::Colour waveformCeilingLine { 0x50DD4444 };  // warm red, ~31% alpha — subtler, closer to Pro-L 2 reference
```

Why 0x50 (31%)?
- At 31% alpha over gradient background (103,99,104):
  R = 103 + 0.31*(221-103) = 140  (was 190)
  G = 99 + 0.31*(68-99) = 89     (was 72)
  B = 104 + 0.31*(68-104) = 93   (was 76)
- Result: muted warm pink (140,89,93) vs reference dark (47,42,46) — still an error but
  far less prominent than current (190,72,76)
- The ceiling LINE label text also uses this color (Colours.h line 351), so both line
  and label become more subtle — consistent with reference style

Screenshot methodology: same as task-349.
Save results to `screenshots/task-350-rmse-results.txt`.

## Dependencies
None (can run independently; if task 349 is merged first the ceiling line will be at crop y≈36 instead of y≈20)
