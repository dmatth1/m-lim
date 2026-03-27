# Task: Tune Wave Left-Edge Idle Gradient Brightness

## Description
The WaveformDisplay draws a 28px-wide "left-edge idle gradient" strip at the left edge
of the waveform area to simulate the input level meter visible in the reference screenshot.

Current rendering (`WaveformDisplay.cpp` lines ~429–438):
- Top: black at 50% alpha → too dark at the left edge
- Bottom: warm-gray 0xffC8B090 at 32% alpha

Pixel analysis of the left 28px of the wave zone (x=0–28, y=40–440):
- M-LIM: R=88, G=88, B=96  (too dark and too warm/neutral)
- Reference: R=98, G=94, B=112  (brighter, more blue)

The black overlay at 50% alpha at the top is pulling the gradient too dark. The reference
shows the left edge is actually *brighter* and bluer than the main waveform at that position,
suggesting it represents an active level meter display, not a dark shadow.

Proposed changes to `WaveformDisplay.cpp` drawBackground():
1. Reduce top black alpha from 0.50f to 0.30f (less darkening at top)
2. Change the bottom warm-gray colour from 0xffC8B090 to 0xffA0A8C8 (more blue, cooler)
   and increase its alpha from 0.32f to 0.42f

These changes should raise the left-edge average from R=88 to approximately R=96–100
while shifting the hue from warm to cool-blue, better matching the reference.

## Produces
None

## Consumes
None

## Relevant Files
Modify: `src/ui/WaveformDisplay.cpp` — lines ~429–438, the left-edge idle gradient block
Read:   `src/ui/Colours.h` — inputWaveform and displayGradientBottom for context

## Acceptance Criteria
- [ ] Run RMSE methodology → Expected: Wave zone RMSE ≤ 16.51% (wave-22 baseline)
- [ ] Run RMSE methodology → Expected: Full RMSE ≤ 19.11% (wave-22 full baseline)
- [ ] Pixel check: left 28px of wave zone R ≥ 94 (brighter than current R=88)

## Tests
None

## Technical Details
The block to change in `WaveformDisplay.cpp::drawBackground()`:
```cpp
// BEFORE:
const float edgeW = 28.0f;
juce::ColourGradient edgeGrad (
    juce::Colours::black.withAlpha (0.50f),          0.0f, area.getY(),
    juce::Colour (0xffC8B090).withAlpha (0.32f),      0.0f, area.getBottom(),
    false);

// AFTER (proposed):
const float edgeW = 28.0f;
juce::ColourGradient edgeGrad (
    juce::Colours::black.withAlpha (0.30f),          0.0f, area.getY(),
    juce::Colour (0xffA0A8C8).withAlpha (0.42f),      0.0f, area.getBottom(),
    false);
```

Pixel verification:
```bash
convert /tmp/task-mlim.png -crop 28x400+0+40 +repage -resize 1x1! \
  -format "%[fx:255*p{0,0}.r],%[fx:255*p{0,0}.g],%[fx:255*p{0,0}.b]" info:
# Target: R ≈ 96, G ≈ 93, B ≈ 111 (matching reference R=98, G=94, B=112)
```

## Dependencies
None
