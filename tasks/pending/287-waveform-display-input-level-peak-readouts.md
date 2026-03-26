# Task 287: WaveformDisplay — Add Stereo Input Peak Level Readout Boxes at Top

## Description
In Pro-L 2, the waveform display shows two small level readout boxes at the top edge of the
waveform area (one per channel) displaying the peak input level in dBFS (e.g., "-0.5 dB" and
"-0.3 dB"). These are positioned in the upper-left corner of the waveform, above the waveform
content.

M-LIM currently shows only the mode-selector label ("FAST") in the top-left corner of the
waveform and has no per-channel peak readout in the waveform display itself. The input peak
values are only visible in the narrow LevelMeter bars outside the waveform.

Adding these peak readout badges to the waveform display improves visual parity with Pro-L 2
and provides a more prominent peak-level indicator.

### Required changes:

#### In `WaveformDisplay.h`:
Add:
```cpp
/** Update the displayed stereo peak level readouts (called from 60fps UI timer). */
void setPeakReadouts (float leftDB, float rightDB);
```

Add private members:
```cpp
float peakReadoutL_ = -96.0f;
float peakReadoutR_ = -96.0f;
```

#### In `WaveformDisplay.cpp`:
1. Implement `setPeakReadouts()`:
```cpp
void WaveformDisplay::setPeakReadouts (float leftDB, float rightDB)
{
    peakReadoutL_ = leftDB;
    peakReadoutR_ = rightDB;
    // No explicit repaint needed — timer repaints at 60fps
}
```

2. In `WaveformDisplay::paint()`, after the other draw calls but before `drawModeSelector()`,
   add a call to a new `drawPeakReadouts()` private method.

3. Implement `drawPeakReadouts()`:
```cpp
void WaveformDisplay::drawPeakReadouts (juce::Graphics& g,
                                         const juce::Rectangle<float>& area) const
{
    // Two small boxes in top-right corner: [L: xxx dB] [R: xxx dB]
    static constexpr float kBoxW = 60.0f;
    static constexpr float kBoxH = 14.0f;
    static constexpr float kGap  =  3.0f;

    auto fmtPeak = [](float db) -> juce::String
    {
        if (db <= -96.0f) return "--- dB";
        return juce::String (db, 1) + " dB";
    };

    juce::String lStr = fmtPeak (peakReadoutL_);
    juce::String rStr = fmtPeak (peakReadoutR_);

    auto drawBox = [&] (float x, float y, const juce::String& text, bool isClipping)
    {
        auto box = juce::Rectangle<float> (x, y, kBoxW, kBoxH);
        g.setColour (MLIMColours::peakLabelBackground.withAlpha (0.85f));
        g.fillRect (box);
        g.setColour (MLIMColours::panelBorder);
        g.drawRect (box, 1.0f);
        g.setFont (juce::Font (MLIMColours::kFontSizeSmall));
        g.setColour (isClipping ? MLIMColours::meterDanger : MLIMColours::peakLabel);
        g.drawText (text, box.reduced (2.0f, 1.0f), juce::Justification::centredRight, false);
    };

    const float rightEdge = area.getRight() - kGap;
    const float topY      = area.getY() + kGap;
    drawBox (rightEdge - kBoxW,            topY, rStr, peakReadoutR_ >= -0.5f);
    drawBox (rightEdge - kBoxW * 2 - kGap, topY, lStr, peakReadoutL_ >= -0.5f);
}
```

#### In `PluginEditor.cpp` `applyMeterData()`:
After the existing `inputMeter_.setPeakHold(...)` call, add:
```cpp
waveformDisplay_.setPeakReadouts (inputPeakL_, inputPeakR_);
```

### Visual target:
- Two compact peak-level badges at the top-right of the waveform area (not top-left where mode selector is)
- Labels: left channel on the left box, right channel on the right box
- Dark background with border; yellow text normally, red text when clipping (≥ -0.5 dBFS)
- "--- dB" shown when no peak has been measured

## Produces
None

## Consumes
None

## Relevant Files
Modify: `M-LIM/src/ui/WaveformDisplay.h` — add `setPeakReadouts()` declaration and private members
Modify: `M-LIM/src/ui/WaveformDisplay.cpp` — implement `setPeakReadouts()` and `drawPeakReadouts()`
Modify: `M-LIM/src/PluginEditor.cpp` — call `waveformDisplay_.setPeakReadouts()` in `applyMeterData()`
Read: `M-LIM/src/ui/Colours.h` — colour constants used for styling
Read: `M-LIM/src/PluginEditor.h` — `inputPeakL_`, `inputPeakR_` member variables

## Acceptance Criteria
- [ ] Run: `cmake --build build --target MLIM_Standalone -j$(nproc) 2>&1 | tail -3` → Expected: build succeeds, exit 0
- [ ] Run: `cd build && ctest --output-on-failure 2>&1 | tail -3` → Expected: all tests pass
- [ ] Visual check: two peak readout badges ("--- dB") appear at the top-right of the waveform area at idle

## Tests
None

## Technical Details
- Peak values come from `inputPeakL_` / `inputPeakR_` in PluginEditor which are already being
  maintained by `updatePeakHold()` / `agePeakHoldCounters()`
- No new data path needed — waveformDisplay_ receives the already-computed peak values
- The clipping threshold of -0.5 dBFS matches `kDangerDB` in LevelMeter
- Badges positioned at top-right to avoid conflict with mode-selector label (top-left)

## Dependencies
None
