# Task: Increase Waveform Idle Fill Opacity to Match Reference Bottom Half

## Description
The waveform idle fill simulation (task-369) adds a semi-transparent blue overlay to the
lower 56% of the waveform to approximate the appearance of active audio content. The
current fill alpha caps at `0.62f` at the bottom edge.

Pixel analysis shows:
- Reference waveform bottom (y=380 of comparison crop): `#7A809B` (R=122, G=128, B=155)
- M-LIM waveform bottom: `#5B6B95` (R=91, G=107, B=149)

M-LIM is still ~25-30 pixel counts too dark in R and G channels at the bottom. Increasing
the idle fill alpha from `0.62f` to `0.80f` would bring the composite appearance closer to
the reference's waveform-fill-brightened bottom.

**Expected RMSE gain:** Wave region −0.2 to −0.3pp.

## Produces
None

## Consumes
None

## Relevant Files
Modify: `src/ui/WaveformDisplay.cpp` — `drawBackground()` method, idle fill alpha
Read: `src/ui/Colours.h` — `inputWaveform` color (`0xCC6878A0`)

## Acceptance Criteria
- [ ] Run: build + screenshot + RMSE → Expected: Wave RMSE ≤ 19.2% (down from 19.44%)
- [ ] Run: Full RMSE → Expected: ≤ 21.1% (no regression)
- [ ] Run: visual check → Expected: lower half of waveform shows subtle blue tint suggesting audio content; top 44% remains dark and unchanged

## Tests
None

## Technical Details

In `src/ui/WaveformDisplay.cpp`, `drawBackground()`, locate the idle fill gradient:

```cpp
juce::ColourGradient fillGrad (
    MLIMColours::inputWaveform.withAlpha (0.0f),   0.0f, fillTop,
    MLIMColours::inputWaveform.withAlpha (0.62f),  0.0f, area.getBottom(),
    false);
```

Change the bottom alpha from `0.62f` to `0.80f`:

```cpp
juce::ColourGradient fillGrad (
    MLIMColours::inputWaveform.withAlpha (0.0f),   0.0f, fillTop,
    MLIMColours::inputWaveform.withAlpha (0.80f),  0.0f, area.getBottom(),
    false);
```

**Calculation:** `inputWaveform` = `0xCC6878A0` = R=104, G=120, B=160 at 80% alpha
composited over `displayGradientBottom` = `0xff506090` = R=80, G=96, B=144:
- Result R: 104×0.80 + 80×0.20 = 83.2+16.0 = 99.2 ≈ 99
- Result G: 120×0.80 + 96×0.20 = 96.0+19.2 = 115.2 ≈ 115
- Result B: 160×0.80 + 144×0.20 = 128+28.8 = 156.8 ≈ 157

This gives approximately `#636373` → actually `#63739D` which is closer to reference `#7A809B`.
Still slightly under-bright but substantially improved from 0.62.

**CAUTION:** The fill only starts at 44% from top (`fillFrac=0.56`), so the top zone is
unaffected. If wave RMSE increases with 0.80, try 0.72 as a midpoint.

## Dependencies
None
