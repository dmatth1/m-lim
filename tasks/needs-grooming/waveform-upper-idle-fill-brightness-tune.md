# Task: Waveform Upper Idle Fill — Reduce Brightness at y=15–30% Zone

## Description
The Wave zone RMSE is 16.79% (crop 600x400+150+50). The main remaining divergence is at
the upper-middle zone of the waveform display (y=100–160 in the 900x500 crop, corresponding
to 15–35% of waveform height).

Pixel comparison at x=400 (waveform center):
```
y=20  (~0%):   M-LIM (45,45,45)  | Ref (43,38,45)  — nearly identical ✓
y=60  (~10%):  M-LIM (54,44,49)  | Ref (44,35,41)  — M-LIM ~10 units brighter
y=100 (~18%):  M-LIM (68,61,72)  | Ref (39,37,42)  — M-LIM ~30 units brighter ✗
y=140 (~28%):  M-LIM (89,92,114) | Ref (38,36,39)  — M-LIM ~55 units brighter ✗
y=180 (~36%):  M-LIM (96,97,121) | Ref (111,125,164) — ref brighter (active waveform peaks)
```

At y=100–140 (18–28% height), M-LIM is significantly brighter than the reference because
the reference has dark waveform-peak shadows there (input waveform fill blocks the gradient),
while M-LIM shows the raw idle fill approximation.

The upper idle fill in `WaveformDisplay::paintIdleContent()` uses:
- Coverage: 15% to 55% height, peak alpha 0.48 at 30% height (`uMid`)
- `inputWaveform.withAlpha(1.0f)` = (104,120,160) — steel blue

Task 420 already reduced the peak alpha from 0.60 to 0.48. A further reduction to 0.32
would reduce the y=140 brightness from ~89,92,114 to approximately ~68,62,74 — somewhat
closer to the reference's dark (~38,36,39) at that zone.

However, reducing this fill will also change the y=180+ area where M-LIM already roughly
matches the reference via other idle fill layers. Measure carefully before/after.

## Produces
None

## Consumes
None

## Relevant Files
Modify: `src/ui/WaveformDisplay.cpp` — upper idle fill block (~lines 309–330), `uMid` alpha
Read:   `src/ui/Colours.h` — `inputWaveform` colour constant

## Acceptance Criteria
- [ ] Run: Wave zone RMSE (crop 600x400+150+50) → Expected: Wave RMSE ≤ 16.79% (improvement or no regression)
- [ ] Run: Full RMSE → Expected: Full ≤ 19.23% (no regression)

## Tests
None

## Technical Details
In `WaveformDisplay.cpp` upper idle fill block:

**Current:**
```cpp
juce::ColourGradient rGrad (
    uFill.withAlpha (0.0f),   0.0f, uTop,
    uFill.withAlpha (0.48f),  0.0f, uMid,   // task-420: peak alpha at 30%
    false);
// ...falling half:
juce::ColourGradient fGrad (
    uFill.withAlpha (0.48f),  0.0f, uMid,   // task-420
    uFill.withAlpha (0.0f),   0.0f, uBot,
    false);
```

**Target (reduce peak alpha):**
```cpp
// Lower uMid alpha to reduce over-brightening at y=18–30%
uFill.withAlpha (0.32f),  // was 0.48 — reduce to 0.32
```

With alpha 0.32, the gradient at 28% height (uMid) will produce:
- background at 28%: ~(79,71,84)
- inputWaveform (104,120,160) at alpha 0.32: (104*0.32+79*0.68, 120*0.32+71*0.68, 160*0.32+84*0.68)
  = (33+54, 38+48, 51+57) = (87,86,108)
- Reference at y=140: 38,36,39 — still diverges, but less than current (89,92,114)

Note: This trade-off inherently degrades the mid-zone match (y=180+) where the current 0.48
alpha helps close the gap. Measure both y=100–160 and y=180–260 zones before committing.
If the overall wave zone RMSE improves (≤ 16.79%), keep the change; otherwise revert.

Also check if reducing `uBot` (the 55% boundary at which the fill fades to zero) to 0.45
could help narrow the active zone:
```cpp
const float uBot = area.getY() + area.getHeight() * 0.45f;  // was 0.55
```

Wave zone RMSE measurement:
```bash
convert /tmp/task-mlim.png -crop 600x400+150+50 +repage /tmp/z-wave.png
convert /tmp/task-ref.png  -crop 600x400+150+50 +repage /tmp/r-wave.png
compare -metric RMSE /tmp/r-wave.png /tmp/z-wave.png /dev/null 2>&1
```

## Dependencies
None
