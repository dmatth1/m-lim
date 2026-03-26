# Task 242: Input Gain — Redesign as Horizontal Handle Matching Pro-L 2

## Description
In Pro-L 2, the input gain control appears as a **floating value badge** in the
bottom-left corner of the waveform display, showing the gain value (e.g., "+10.5 dB").
A horizontal drag gesture changes the gain. This is visually integrated and unobtrusive.

M-LIM currently shows a tall vertical JUCE `Slider` (LinearVertical style) with a
white circle thumb overlaid on the left edge of the waveform area, plus a "GAIN"
label above and a value label below. This vertical slider is visually very different
from the Pro-L 2 reference.

**Fix**: Redesign the input gain control as follows:
1. Replace the vertical `inputGainSlider_` with a **horizontal slider** positioned at
   the very bottom-left of the waveform area (inside the waveform bounds).
2. Size it to ~80 px wide × 20 px tall, positioned in the bottom-left corner of the
   waveform display, over the waveform content.
3. Render it as a small labelled handle: a rounded rectangle badge (matching
   `MLIMColours::peakLabelBackground` fill, `MLIMColours::panelBorder` outline) showing
   the gain value in yellow text (e.g., "+0.0 dB").
4. Remove the "GAIN" label above the slider and the `inputGainValueLabel_` below — the
   badge handles the display.
5. Horizontal drag (left = decrease gain, right = increase gain) changes the value.
   Use `juce::Slider::LinearHorizontal` style with `NoTextBox`.

The APVTS attachment (`inputGainAttach_`) must remain wired to `ParamID::inputGain`.

## Produces
None

## Consumes
None

## Relevant Files
Modify: `src/PluginEditor.cpp` — replace vertical slider + labels with horizontal badge slider
Modify: `src/PluginEditor.h` — remove `inputGainLabel_`, update `inputGainValueLabel_` usage; update layout constants
Read:   `src/ui/Colours.h` — colours for badge styling
Read:   `src/Parameters.h` — ParamID::inputGain for attachment

## Acceptance Criteria
- [ ] Run: build, launch standalone → Expected: no tall vertical slider overlaid on left waveform edge; instead, a small floating badge in bottom-left of waveform showing gain value
- [ ] Run: build, drag the gain badge left/right → Expected: gain value changes smoothly; APVTS parameter updates
- [ ] Run: `cd build && ctest --output-on-failure` → Expected: all tests pass

## Tests
None (visual/interaction change; no new testable logic)

## Technical Details
In `PluginEditor.h`:
- Remove `inputGainLabel_` member
- Change `kGainSliderW = 20` and `kGainLabelH = 14` to `kGainBadgeW = 80`, `kGainBadgeH = 20`
- Keep `inputGainValueLabel_` for the badge text readout
- Keep `inputGainSlider_` as LinearHorizontal

In `PluginEditor.cpp resized()`:
```cpp
// Position the gain badge in the bottom-left of the waveform area
auto gainBadge = bounds  // waveformDisplay bounds
    .getLocalBounds()
    .withBottomY(waveformDisplay_.getBottom())  // bottom of waveform
    ...
```
Use `gainOverlay = bounds.withWidth(kGainBadgeW).withHeight(kGainBadgeH)` placed at
bottom-left of the waveform bounds.

Style the slider: set `setSliderStyle(juce::Slider::LinearHorizontal)` with
`NoTextBox`, transparent background. The `onValueChange` lambda updates
`inputGainValueLabel_` text displayed as a child label on top of the badge area.

## Dependencies
None
