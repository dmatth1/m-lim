# Task 363: Left Meter Region — Waveform Edge Color Improvement

## Description
The left meter sub-region crop (30x378+0+30) measures 23.50% RMSE. This region is the leftmost
30px of the waveform display area. In the reference (Pro-L 2 with audio playing), this area shows
an active input level meter with warm amber/blue gradient at about half-fill.

The current M-LIM waveform background at the left edge (~x=0-30) shows the waveform display
background color (dark navy). The reference at this x position shows level meter colors: amber
yellow at the top (active level zone) transitioning to blue/purple at the bottom.

**Approach:** Add a subtle idle structural gradient to the leftmost region of the WaveformDisplay
component, mimicking the visual appearance of an input level meter at idle/low level. This is a
cosmetic technique — not real metering — to reduce the structural RMSE gap with the reference.

Specifically: draw a narrow (~10-14px wide) gradient strip on the left edge of the waveform area
using meter-like colors at low alpha (20-30%), so it adds some warm-amber-to-blue character without
obscuring the waveform content.

## Produces
None

## Consumes
None

## Relevant Files
Modify: `M-LIM/src/ui/WaveformDisplay.cpp` — add left-edge idle gradient in `drawBackground()`
Read: `M-LIM/src/ui/Colours.h` — use meterSafe/meterWarning/meterDanger colors for consistency

## Acceptance Criteria
- [ ] Run: build and capture screenshot → Expected: app renders without crash
- [ ] Run: left meters RMSE → Expected: improvement from 23.50% (goal ≤22.50%)
- [ ] Visual check: gradient is subtle and does not obscure the main waveform content

## Tests
None

## Technical Details

**In WaveformDisplay.cpp `drawBackground()` method, add at the very end (or as a separate call):**

```cpp
// Left-edge idle gradient — provides warm amber→blue character matching Pro-L 2 input meter region
// Drawn at low alpha to blend naturally with waveform content
const float edgeW = 12.0f;  // narrow strip, same as input meter width
juce::ColourGradient edgeGrad (
    MLIMColours::meterWarning.withAlpha (0.22f), 0.0f, area.getY(),          // amber at top
    MLIMColours::meterSafe.withAlpha (0.22f),    0.0f, area.getBottom(),     // steel-blue at bottom
    false);
edgeGrad.addColour (0.3f, MLIMColours::meterWarning.withAlpha (0.18f));  // warm mid-zone
g.setGradientFill (edgeGrad);
g.fillRect (area.getX(), area.getY(), edgeW, area.getHeight());
```

**Fine-tuning guidance:**
- If RMSE improves but looks visually odd, reduce alpha from 0.22f toward 0.12f
- The gradient Y span should cover the full waveform height (not just the active area)
- Strip width: 10-14px is appropriate (the crop is 30px wide so it covers the first 12px)
- Color reference: at y=30% from top, reference shows ~#CA9B35 (warm amber)
  at y=70% from top, reference shows ~#7579 94 (steel blue-purple)

**RMSE measurement:**
```bash
compare -metric RMSE \
    <(convert /tmp/ref363.png -crop 30x378+0+30 +repage png:-) \
    <(convert /tmp/task363-mlim.png -crop 30x378+0+30 +repage png:-) \
    /dev/null 2>&1
```

Save full results to `screenshots/task-363-rmse-results.txt`.

## Dependencies
None (can run in parallel with tasks 361 and 362)
