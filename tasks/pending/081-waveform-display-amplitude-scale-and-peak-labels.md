# Task 081: WaveformDisplay Amplitude Mapping and Peak Label Rendering

## Description
Two separate issues in the already-implemented `WaveformDisplay` component:

**Issue 1 — Wrong amplitude mapping**: `levelToY()` maps input/output levels **linearly** (0.0–1.0 → bottom–top). But `drawBackground()` draws grid lines using a **dB/GR scale** (`frac = (-db) / kMaxGRdB`). These are two different coordinate systems — the grid lines at -6 dB, -12 dB etc. do NOT correspond to where a -6 dBFS or -12 dBFS signal would appear in the waveform fills. The waveform fill and the dB grid are misaligned.

The fix: convert linear amplitude to dB before mapping to Y position:
```cpp
float dBFS = (linear > 0.0f) ? juce::Decibels::gainToDecibels(linear) : -kMaxGRdB;
float frac  = juce::jlimit(0.0f, 1.0f, (-dBFS) / kMaxGRdB);
return area.getBottom() - frac * area.getHeight();  // (inverted: 0dB at top)
```
Wait — actually the display should use a consistent scale. Decide: use dB scale throughout (both grid and waveform), so a -6 dBFS signal appears at the -6 dB grid line.

**Issue 2 — Peak labels are plain text (missing box background)**: `drawPeakMarkers()` draws the gain reduction label as plain `g.drawText()` with just the `peakLabel` colour and no background box. In the Pro-L 2 reference, peak markers are **yellow/gold rounded-rectangle boxes with dark text inside** — highly readable against the waveform. Current floating text will be unreadable on a busy waveform background.

Reference: `/reference-docs/reference-screenshots/prol2-main-ui.jpg` (peak labels visible as yellow boxes showing e.g. "-0.5 dB", "-0.3 dB"), `/reference-docs/video-frames/v1-0040.png`.

## Produces
None

## Consumes
WaveformDisplayInterface
ColoursDefinition

## Relevant Files
Modify: `M-LIM/src/ui/WaveformDisplay.cpp` — fix `levelToY()` to use dB mapping, fix `drawPeakMarkers()` to render boxes

## Acceptance Criteria
- [ ] Run: `cd M-LIM && cmake --build build --target MLIM_VST3 -j$(nproc) 2>&1 | tail -3` → Expected: builds successfully
- [ ] Run: `grep -c "gainToDecibels\|Decibels\|dBFS\|log10" M-LIM/src/ui/WaveformDisplay.cpp` → Expected: at least 1 match (dB conversion present)
- [ ] Run: `grep -c "fillRoundedRectangle\|fillRect.*label\|peakLabel" M-LIM/src/ui/WaveformDisplay.cpp` → Expected: at least 2 matches (box background drawn)

## Tests
None (visual component)

## Technical Details

**Fix 1 — `levelToY()` dB mapping:**
```cpp
float WaveformDisplay::levelToY(float linear, const juce::Rectangle<float>& area) const noexcept
{
    // Convert linear amplitude to dB, then map to Y coordinate
    // 0 dBFS → top of area, -kMaxGRdB (e.g. -24) → bottom
    float dBFS = (linear > 1e-6f)
        ? juce::Decibels::gainToDecibels(juce::jlimit(1e-6f, 1.0f, linear))
        : -kMaxGRdB;
    dBFS = juce::jlimit(-kMaxGRdB, 0.0f, dBFS);
    float frac = (-dBFS) / kMaxGRdB;  // 0 = at top (0 dBFS), 1 = at bottom (-kMaxGRdB)
    return area.getY() + frac * area.getHeight();
}
```
With this fix, a signal at -6 dBFS will appear at the -6 dB grid line as expected.

**Fix 2 — peak label boxes in `drawPeakMarkers()`:**
Replace plain text draw with a rounded box + text:
```cpp
// Draw yellow box background
auto bgRect = juce::Rectangle<float>(x - 22.0f, y + 2.0f, 44.0f, 14.0f);
g.setColour(MLIMColours::peakLabel.withAlpha(0.85f));
g.fillRoundedRectangle(bgRect, 3.0f);

// Draw label text in dark/black on top of box
g.setColour(juce::Colour(0xff1A1A1A));  // dark text on yellow box
g.setFont(juce::Font(9.0f));
g.drawText(label, bgRect, juce::Justification::centred, false);
```
The box size should be proportional to the label text. Consider using `g.getCurrentFont().getStringWidthFloat(label) + 8.0f` for the box width.

**kMaxGRdB value:** Verify `kMaxGRdB` in `WaveformDisplay.h` — it should be `24.0f` (matching the grid range 0 to -24 dB). If different, the Y coordinates of grid lines and waveform fills must agree.

## Dependencies
Requires task 022 (WaveformDisplay component)
