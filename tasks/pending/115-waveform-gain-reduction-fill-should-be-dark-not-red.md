# Task 115: Waveform GR Fill Is Bright Red — Reference Shows Near-Black

## Description
`WaveformDisplay::drawGainReduction()` uses `MLIMColours::gainReduction.withAlpha(0.75f)` which renders as bright red (`#FF4444` at 75% opacity) filling the gain reduction zone from the top of the waveform.

The reference frames (`v1-0008.png`, `v1-0009.png`, `v1-0005.png`) clearly show the gain reduction area in the waveform display as **very dark (near-black)** spikes from the top — NOT red. The red color is reserved for the separate `GainReductionMeter` bar component on the right side of the waveform.

In the reference:
- The waveform GR area appears as approximately `#0A0A10` to `#12121C` — very dark with a slight cool tint
- The contrast between the GR dark fill and the blue-grey input fill creates the visual impression of "peaks being cut"
- The GainReductionMeter bar (separate component, `GainReductionMeter.cpp`) correctly stays red

**Fix:**
In `WaveformDisplay.cpp::drawGainReduction()`, change the colour used for the fill:
```cpp
// Before:
g.setColour (MLIMColours::gainReduction.withAlpha (0.75f));

// After:
g.setColour (juce::Colour (0xD0060810));  // near-black with very slight cool tint, 82% opacity
```

Do NOT change `MLIMColours::gainReduction` — it is also used by `GainReductionMeter.cpp` where red is correct.

## Produces
None

## Consumes
None

## Relevant Files
Modify: `M-LIM/src/ui/WaveformDisplay.cpp` — change colour in `drawGainReduction()` only
Read: `/reference-docs/video-frames/v1-0009.png` — GR spikes in waveform are clearly dark/black
Read: `/reference-docs/video-frames/v1-0008.png` — same, heavy GR visible as dark areas from top
Read: `/reference-docs/video-frames/v1-0005.png` — gentle GR shows dark peaks above output envelope
Skip: `M-LIM/src/ui/GainReductionMeter.cpp` — this component correctly uses red

## Acceptance Criteria
- [ ] Run: `grep -A3 'gainReduction.withAlpha\|drawGainReduction' M-LIM/src/ui/WaveformDisplay.cpp | head -20` → Expected: no reference to `gainReduction.withAlpha` (replaced with literal dark colour)
- [ ] Run: `cmake --build build -j$(nproc) 2>&1 | tail -3` → Expected: build succeeds
- [ ] Run: `grep 'gainReduction' M-LIM/src/ui/GainReductionMeter.cpp | head -5` → Expected: still references `MLIMColours::gainReduction` (red — not changed)

## Tests
None

## Technical Details
The change is a single line in `WaveformDisplay::drawGainReduction()`:
```cpp
void WaveformDisplay::drawGainReduction (juce::Graphics& g,
                                          const juce::Rectangle<float>& area) const
{
    // ...path building code unchanged...

    // Change this:
    g.setColour (MLIMColours::gainReduction.withAlpha (0.75f));
    // To:
    g.setColour (juce::Colour (0xD0060810));  // near-black, 82% opaque
    g.fillPath (path);
}
```

The `MLIMColours::gainReduction` constant should remain `0xffFF4444` as it is also used by `GainReductionMeter.cpp`. Only the inline usage in `WaveformDisplay.cpp` needs to change.

## Dependencies
None
