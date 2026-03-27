# Task 421: Waveform Upper Zone — Darken, Reduce Purple, and Reduce Upper Idle Fill

## Description
The upper waveform zone (top 16%, y=40–120 in the 900x500 crop) shows a blue-purple cast
and is significantly brighter than the reference at y=15–30% height.

Pixel evidence:
- Upper zone avg: M-LIM `#3E353D` (R=62, G=53, B=61) vs Ref `#322B2C` (R=50, G=43, B=44)
- y=100 (18%): M-LIM (68,61,72) vs Ref (39,37,42) — M-LIM ~30 units brighter
- y=140 (28%): M-LIM (89,92,114) vs Ref (38,36,39) — M-LIM ~55 units brighter

Two fixes combined (both address the same file and interact):

**Fix 1** — Darken `displayGradientTop` in `Colours.h`:
```cpp
// current:  0xff302528  (+9 vs previous, task-420)
// target:   0xff282020  (-8 units: R=40,G=32,B=32)
```

**Fix 2** — Reduce upper idle fill peak alpha in `WaveformDisplay.cpp` (lines 318, 324):
```cpp
// current: uFill.withAlpha(0.48f)   (task-420 reduced from 0.60)
// target:  uFill.withAlpha(0.32f)   (further reduction to suppress y=18–28% over-brightness)
```

These two fixes must be applied together in one task to avoid a follow-up conflict on the
same file. Measure Wave zone RMSE after each change and keep whichever combination
minimises RMSE without regressing Full RMSE beyond 19.50%.

## Produces
None

## Consumes
None

## Relevant Files
Modify: `src/ui/Colours.h` — `displayGradientTop` (~line 22)
Modify: `src/ui/WaveformDisplay.cpp` — upper idle fill alpha at lines ~318, 324

## Acceptance Criteria
- [ ] Run: `cmake --build build --target MLIM_Standalone_Standalone -j$(nproc) 2>&1 | tail -3` → Expected: build succeeds, exit 0
- [ ] Run: Wave zone RMSE (crop 600x400+150+50) → Expected: ≤ current wave baseline (improvement or no regression)
- [ ] Run: Full RMSE → Expected: ≤ 19.50% (no significant regression)

## Tests
None

## Technical Details
Build only Standalone: `cmake --build build --target MLIM_Standalone_Standalone -j$(nproc)`.
Set CCACHE_DIR=/build-cache/ccache before building.

Upper idle fill block in `WaveformDisplay.cpp` (~lines 309–330):
```cpp
juce::ColourGradient rGrad (
    uFill.withAlpha (0.0f),   0.0f, uTop,
    uFill.withAlpha (0.32f),  0.0f, uMid,  // was 0.48 (task-420), reduce to 0.32
    false);
// ...falling half:
juce::ColourGradient fGrad (
    uFill.withAlpha (0.32f),  0.0f, uMid,  // was 0.48
    uFill.withAlpha (0.0f),   0.0f, uBot,
    false);
```

## Dependencies
None
