# Task 423: Waveform Left-Edge Idle Gradient — Colour Correction

## Description
`WaveformDisplay.cpp` (~line 433) renders a left-edge idle gradient over the first 28px of
the waveform area using `juce::Colour (0xffD8ACD0).withAlpha(0.52f)` — a warm pink-lavender
(R=216, G=172, B=208). This creates a visible pink tint at x=30–58 (waveform left edge)
that differs from the reference, which shows neutral blue-gray waveform gradient tones there.

Fix: either replace the pink colour with a neutral dark overlay or remove the gradient entirely.

**Option A** (recommended — try first): remove the edgeGrad block entirely.
**Option B**: replace with neutral dark overlay:
```cpp
juce::ColourGradient edgeGrad (
    juce::Colours::black.withAlpha (0.20f),  0.0f, area.getY(),
    juce::Colours::black.withAlpha (0.05f),  0.0f, area.getBottom(),
    false);
```

Measure Wave zone RMSE before and after both options and keep whichever produces
improvement or no regression.

## Produces
None

## Consumes
None

## Relevant Files
Modify: `src/ui/WaveformDisplay.cpp` — left-edge idle gradient block (~lines 429–438)
Read: `src/ui/Colours.h` — `displayGradientTop`, `displayGradientBottom`, `inputWaveform`

## Acceptance Criteria
- [ ] Run: `cmake --build build --target MLIM_Standalone_Standalone -j$(nproc) 2>&1 | tail -3` → Expected: build succeeds, exit 0
- [ ] Run: Wave zone RMSE (crop 600x400+150+50) → Expected: ≤ prior Wave baseline (no regression)
- [ ] Run: Full RMSE → Expected: ≤ 19.50%
- [ ] Visual: no visible pink tint at waveform left edge (x=30–58 in 900x500 crop)

## Tests
None

## Technical Details
The left-edge gradient is in the `paintIdleContent()` method. After task 422, the
`0xffD8ACD0` colour will also be extracted to a Colours.h constant (task 433), so
this task should eliminate usage regardless of whether it's extracted first.

Build Standalone only: `cmake --build build --target MLIM_Standalone_Standalone -j$(nproc)`.

## Dependencies
Requires task 422
