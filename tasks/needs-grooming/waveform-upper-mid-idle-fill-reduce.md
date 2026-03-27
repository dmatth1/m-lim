# Task: Waveform display — reduce upper-mid idle fill brightness (Wave zone RMSE)

## Description
The wave zone (640×500 crop at x=0) shows the waveform display upper-middle region
(y=100–250, corresponding to 20–50% height / the -3 to -12 dBFS zone) is too bright:

- M-LIM 20–50% average: #61657D (R=97, G=101, B=125)
- Reference 20–50% average: #51566D (R=81, G=86, B=109)
- Gap: M-LIM is ~16R, +15G, +16B too bright in this region

This is caused by the "upper idle fill" added in task-403 (covers y=15%–55%, peak alpha 0.60
at midpoint 30%). The 0.60 peak alpha is slightly too high, making the upper quarter too bright.

Additionally, the very top (y=0–100) is slightly too dark:
- M-LIM top-20% avg: #302B2F (R=48, G=43, B=47)
- Reference top-20% avg: #3A3235 (R=58, G=50, B=53)
- Gap: M-LIM is ~10 units too dark at top

Adjustments:
1. Reduce the upper idle fill peak alpha from 0.60 to ~0.48
2. Slightly brighten the displayGradientTop color to close the 10-unit top gap

## Produces
None

## Consumes
None

## Relevant Files
Modify: `M-LIM/src/ui/WaveformDisplay.cpp` — `drawBackground()` upper idle fill block (lines 310–328)
Modify: `M-LIM/src/ui/Colours.h` — `displayGradientTop` constant (line 22)

## Acceptance Criteria
- [ ] Run: build → Expected: compiles without error
- [ ] Run: Wave zone RMSE (crop 640x500+0+0) → Expected: Wave RMSE < 16.51% baseline
- [ ] Run: Full RMSE → Expected: Full ≤ 19.11% (no regression)

## Tests
None

## Technical Details
**Waveform upper idle fill (WaveformDisplay.cpp ~lines 310–328):**

Current:
```cpp
juce::ColourGradient rGrad (
    uFill.withAlpha(0.0f),   0.0f, uTop,
    uFill.withAlpha(0.60f),  0.0f, uMid,   // <-- too bright at 20-50% zone
    false);
...
juce::ColourGradient fGrad (
    uFill.withAlpha(0.60f),  0.0f, uMid,
    uFill.withAlpha(0.0f),   0.0f, uBot,
    false);
```

Target: reduce peak alpha from 0.60 to ~0.48:
```cpp
    uFill.withAlpha(0.48f),  // was 0.60 — reduces 20-50% brightness by ~8 units
```

**displayGradientTop (Colours.h line 22):**

Current: `0xff271F22` (R=39, G=31, B=34)
Target: ~`0xff302528` (R=48, G=37, B=40) — adds ~9 units to top to close the 10-unit gap

Measure the wave zone sub-regions before and after:
```bash
convert /tmp/mlim.png -crop 640x100+0+0 +repage -resize 1x1 txt:-   # top 20%
convert /tmp/mlim.png -crop 640x150+0+100 +repage -resize 1x1 txt:- # 20-50%
```

## Dependencies
None
