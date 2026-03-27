# Task: Waveform Left-Edge Idle Gradient — Colour Correction for Left Zone

## Description
The waveform display renders a "left-edge idle gradient" over the first 28px of the waveform
area (`WaveformDisplay.cpp` ~line 433) to simulate the input meter region appearance.
This renders ON TOP OF the waveform display (which starts at x=30 in the 900x500 crop).

Current left-edge gradient:
```cpp
const float edgeW = 28.0f;
juce::ColourGradient edgeGrad (
    juce::Colours::black.withAlpha (0.10f),          0.0f, area.getY(),
    juce::Colour (0xffD8ACD0).withAlpha (0.52f),      0.0f, area.getBottom(),
    false);
```

The colour `0xffD8ACD0` is a warm pink-lavender (R=216, G=172, B=208). At alpha 0.52, this
creates a pinkish hue at x=30–58 (waveform left edge). This does NOT affect the Left zone
RMSE measurement (crop 30x378+0+30 = input meter only), but it creates a visible pink tint
at the waveform left edge that differs from the reference.

In the reference, the waveform extends right to the edge — there is no input meter strip
and no left-edge overlay. The very left of the reference waveform at x=0–30 shows the
waveform gradient content (cool blue-gray, ~104,121,160 range).

The waveform left edge at x=30–58 in M-LIM should show neutral/blue-gray tones matching
the waveform gradient, not a warm pink overlay.

Pixel check needed: at x=35, y=200–300 in the 900x500 M-LIM crop (inside waveform, first
5px after input meter boundary), compare to reference at same position.

## Produces
None

## Consumes
None

## Relevant Files
Modify: `src/ui/WaveformDisplay.cpp` — left-edge idle gradient block (~lines 429–438)
Read:   `src/ui/Colours.h` — `displayGradientTop`, `displayGradientBottom`, `inputWaveform`

## Acceptance Criteria
- [ ] Run: Wave zone RMSE (crop 600x400+150+50) → Expected: Wave RMSE ≤ 16.79% (improvement or no regression)
- [ ] Run: Full RMSE → Expected: Full ≤ 19.23% (no regression)
- [ ] Visual: waveform left edge (x=30–58) shows neutral blue-gray tones, no visible pink tint

## Tests
None

## Technical Details
The left-edge overlay purpose is to simulate the appearance of the input meter region
bleeding into the waveform — simulating a Pro-L 2 style where the meter and waveform
share the same visual space. However the current `0xffD8ACD0` (warm pink) colour is wrong
for this purpose.

**Option A: Change to neutral dark overlay (suppress the gradient)**
```cpp
juce::ColourGradient edgeGrad (
    juce::Colours::black.withAlpha (0.20f),           0.0f, area.getY(),
    juce::Colours::black.withAlpha (0.05f),            0.0f, area.getBottom(),
    false);
```
This adds a slight top-darkening to blend the input meter edge into the waveform.

**Option B: Remove the overlay entirely**
```cpp
// Remove the edgeGrad block entirely
```
Without the overlay, the waveform gradient shows at x=30 without modification, which
is cleaner and more accurate to the reference.

Measure Wave zone RMSE before and after both options. If Option B gives ≤ 16.79%, use it.
If RMSE increases, try Option A.

## Dependencies
None
