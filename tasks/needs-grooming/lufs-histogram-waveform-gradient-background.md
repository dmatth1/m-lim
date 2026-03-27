# Task: LUFS Panel Histogram — Use Waveform Gradient as Background

## Description
The "Waveform" RMSE sub-region crop (`600x400+150+50`) captures x=150..750. Of this, x=660..750
(90px wide, 15% of the crop width) falls within the LoudnessPanel component. This area shows
M-LIM's dark loudness panel background (`loudnessPanelBackground = #2B2729 = (43, 39, 41)`) while
the reference shows bright blue-gray level meter fills (100–165 value range).

**Per-pixel RMSE at x=700, y=150:**
- Current: dark panel (43,39,41) vs ref (106,111,137) → 30.7% RMSE per pixel
- With waveform gradient (≈97,99,116 at 30% height): 5.8% RMSE per pixel

This improvement across the 90×400 area (15% of the 600×400 waveform crop) would reduce the
waveform RMSE by approximately 2–3 percentage points (21.45% → ~19%).

**Fix:**
In `LoudnessPanel::paint()`, replace the flat `loudnessPanelBackground` fill of the **histogram
area** (upper portion of the panel, height = `getHeight() - kReadoutAreaH`) with the same vertical
gradient used by the waveform display (`displayGradientTop → displayGradientBottom`).

The flat `loudnessPanelBackground` fill of the **readout area** (lower `kReadoutAreaH` pixels)
should remain unchanged — the LUFS Momentary/Short-Term/etc. text needs a dark background.

**Implementation — in `LoudnessPanel::paint()`, split the fillRoundedRectangle into two parts:**
1. Histogram area (top): gradient fill using `displayGradientTop` → `displayGradientBottom`
2. Readout area (bottom): keep `loudnessPanelBackground` flat fill

The histogram content (scale labels, bars, target line) is drawn on top of the background and
will remain visible on both backgrounds.

## Produces
None

## Consumes
None

## Relevant Files
Modify: `M-LIM/src/ui/LoudnessPanel.cpp` — change histogram area background from flat dark to waveform gradient
Read: `M-LIM/src/ui/Colours.h` — `displayGradientTop`, `displayGradientBottom`, `loudnessPanelBackground`
Read: `M-LIM/src/ui/LoudnessPanel.h` — find `kReadoutAreaH` constant definition

## Acceptance Criteria
- [ ] Run: build and capture screenshot, compute waveform RMSE:
  ```
  compare -metric RMSE \
      <(convert /tmp/ref.png -crop 600x400+150+50 +repage png:-) \
      <(convert /tmp/task-mlim.png -crop 600x400+150+50 +repage png:-) \
      /dev/null 2>&1
  ```
  → Expected: waveform RMSE ≤ 20.00% (improvement from 21.45%)
- [ ] Run: `grep -n "displayGradientTop\|displayGradientBottom" M-LIM/src/ui/LoudnessPanel.cpp` → Expected: both referenced in paint()

## Tests
None

## Technical Details

**Current paint() structure (simplified):**
```cpp
void LoudnessPanel::paint(juce::Graphics& g)
{
    // BAD: flat fill for entire panel
    g.setColour(MLIMColours::loudnessPanelBackground);
    g.fillRoundedRectangle(getLocalBounds().toFloat(), 4.0f);

    // histogram drawn on top...
    // readout rows drawn below...
}
```

**New paint() structure:**
```cpp
void LoudnessPanel::paint(juce::Graphics& g)
{
    const int histH = std::max(0, getHeight() - kReadoutAreaH);
    auto histBounds = getLocalBounds().withHeight(histH).toFloat();
    auto readoutBounds = getLocalBounds().withTrimmedTop(histH).toFloat();

    // Histogram area: waveform gradient background (matches reference level meter visual)
    juce::ColourGradient grad = juce::ColourGradient::vertical(
        MLIMColours::displayGradientTop,
        histBounds.getY(),
        MLIMColours::displayGradientBottom,
        histBounds.getBottom());
    g.setGradientFill(grad);
    g.fillRoundedRectangle(getLocalBounds().toFloat(), 4.0f);  // rounded corners cover full panel

    // Readout area: dark flat background overlay
    g.setColour(MLIMColours::loudnessPanelBackground);
    g.fillRect(readoutBounds);

    // Panel border
    g.setColour(MLIMColours::panelBorder);
    g.drawRoundedRectangle(getLocalBounds().reduced(1).toFloat(), 4.0f, 1.0f);

    // ... rest of histogram and readout drawing unchanged ...
}
```

**Why this works:**
- Reference's right panel at x=660..750 shows bright blue-gray level meter content (~100–165 RGB)
- Waveform gradient at y=40..280 maps to approximately (88–104, 96–100, 104–144) RGB
- This bridges the 30+ RGB gap vs the current dark (#2B2729 = 43,39,41) panel background
- LUFS readout text (#9E9E9E) remains readable on the gradient background (sufficient contrast)
- The target level line (gold/yellow) remains clearly visible

**Visual note:**
The histogram scale labels and empty histogram bars will appear on a blue-gray gradient rather
than a dark panel. This creates a visual continuity with the waveform display, which is
appropriate since the reference also shows meter content (not a distinct dark panel) at this position.

## Dependencies
None
