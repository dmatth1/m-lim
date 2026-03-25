# Task 111: Input Gain Slider Is Too Wide and Visually Intrusive

## Description
The input gain slider is currently a full-height vertical slider overlaid on the left edge of the waveform display at `kGainSliderW = 34` pixels wide with a prominent blue track and large thumb. This is too intrusive — it significantly overlaps the waveform display area.

The reference (`prol2-main-ui.jpg`, `v1-0020.png`, `v1-0030.png`) shows the input gain control as a very narrow and subtle overlay:
- A small **"GAIN"** label in the top-left corner of the waveform
- A small numeric readout showing the current gain value (e.g., `+10.5`, `+35.1`) as a compact pill/label
- The actual drag control is minimal — just a narrow vertical strip with a thin track

**Specific problems with the current implementation:**
1. Width `kGainSliderW = 34` px is too wide — should be ≤ 20 px
2. The blue `sliderFill` track colour makes it visually clash with the waveform
3. The slider thumb (10×10 rounded rect) is too large — should be a thin horizontal handle (e.g., 16×4)
4. The "GAIN" text label and value label take up 14+12 = 26px of vertical space at the top

**Fix:**
- Reduce `kGainSliderW` from 34 to 20 in `PluginEditor.h`
- In `PluginEditor.cpp`, change the slider's track colour to a semi-transparent neutral (`0x40FFFFFF`) instead of the accent blue
- Change the thumb colour to `0xC0FFFFFF` (slightly transparent white) to blend better

The slider should feel like a thin ruler handle on the waveform edge, not a prominent control.

## Produces
None

## Consumes
None

## Relevant Files
Modify: `M-LIM/src/PluginEditor.h` — reduce `kGainSliderW` from 34 to 20
Modify: `M-LIM/src/PluginEditor.cpp` — update slider track colour and thumb colour setup
Read: `/reference-docs/video-frames/v1-0020.png` — shows compact "Gain +9.9" label in top-left
Read: `/reference-docs/reference-screenshots/prol2-main-ui.jpg` — shows "GAIN" label and "+10.5" compact display

## Acceptance Criteria
- [ ] Run: `grep 'kGainSliderW' M-LIM/src/PluginEditor.h` → Expected: value is 20 (not 34)
- [ ] Run: `cmake --build build -j$(nproc) 2>&1 | tail -3` → Expected: build succeeds

## Tests
None

## Technical Details
In `PluginEditor.cpp` constructor, change these lines:
```cpp
inputGainSlider_.setColour (juce::Slider::backgroundColourId, juce::Colour (0xff1A1A2E));
inputGainSlider_.setColour (juce::Slider::trackColourId,      MLIMColours::sliderFill.withAlpha (0.8f));
inputGainSlider_.setColour (juce::Slider::thumbColourId,      juce::Colour (0xffE0E0E0));
```
To:
```cpp
inputGainSlider_.setColour (juce::Slider::backgroundColourId, juce::Colour (0x30FFFFFF));
inputGainSlider_.setColour (juce::Slider::trackColourId,      juce::Colour (0x50FFFFFF));
inputGainSlider_.setColour (juce::Slider::thumbColourId,      juce::Colour (0xC0FFFFFF));
```

And in `PluginEditor.h`:
```cpp
static constexpr int kGainSliderW    = 20;  // was 34
```

## Dependencies
None
