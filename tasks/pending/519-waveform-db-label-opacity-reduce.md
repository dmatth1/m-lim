# Task 519: Waveform Display — Reduce dB Overlay Label Opacity

## Description
The dB overlay labels on the waveform display (drawn as left-aligned text at each grid line) currently render at `textSecondary.withAlpha(0.55f)`. The reference Pro-L 2 shows these labels as very subtle, barely visible markers — approximately 0.30-0.35 opacity.

At 0.55 alpha, the labels (#9E9E9E at 55% alpha) create a more noticeable text overlay than the reference shows. Reducing this to 0.35 will make the grid labels more subtle and closer to the reference appearance.

**Change**: In `WaveformDisplay.cpp::drawBackground()`, change the dB overlay labels from:
```cpp
g.setColour (MLIMColours::textSecondary.withAlpha (0.55f));
```
to:
```cpp
g.setColour (MLIMColours::textSecondary.withAlpha (0.35f));
```

## Produces
None

## Consumes
None

## Relevant Files
Modify: `src/ui/WaveformDisplay.cpp` — label opacity in `drawBackground()`, near the "dB overlay labels" comment block (approximately line 358)

## Acceptance Criteria
- [ ] Run: `cmake --build build --target MLIM_Standalone -j$(nproc)` → Expected: build succeeds, exit 0
- [ ] Run: capture waveform area screenshot → Expected: dB labels (-3 dB, -6 dB, etc.) are more subtle/faint than before, still readable but less prominent

## Tests
None

## Technical Details
Single alpha value change: `0.55f` → `0.35f` in the dB label rendering section of `drawBackground()`.

The grid lines themselves use `MLIMColours::waveformGridLine.withAlpha(0.25f)` — leave this unchanged.

Expected RMSE improvement: ~0.2pp for Waveform zone (currently ~16.51%), negligible for other zones.

## Dependencies
None
