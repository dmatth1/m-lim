# Task 188: LevelMeter — Add Peak Hold Numeric Labels at Top of Meter

## Description
The reference Pro-L 2 shows small numeric peak-hold readouts at the **very top** of the input and output level meter columns. These floating peak labels (e.g., "-1.0", "-6.5") give the user a precise numeric reading of the peak hold value alongside the visual bar indicator.

Reference frames confirming this element:
- `/reference-docs/reference-screenshots/prol2-main-ui.jpg` — top-right of the display shows small value boxes above the meter bars
- `/reference-docs/reference-screenshots/prol2-intro.jpg` — similar small boxes visible at top of right metering panel

The current `LevelMeter` implementation:
- Draws a white peak hold **line** at the peak position (correct)
- Does **not** draw any numeric label showing the dB peak value

**Required addition:** Draw a compact numeric label at the top of each channel bar showing the current peak hold value in dBFS, formatted to one decimal place (e.g., "-6.3").

Layout:
- Reserve a small strip (`kPeakLabelH ≈ 14px`) at the top of the meter for the numeric readout
- Draw the peak dB value centred above each bar in 9pt font
- Colour: `meterDanger` (red) when peak ≥ `kDangerDB` (-0.5 dBFS), `meterWarning` (yellow) when peak ≥ `kWarnDB` (-3 dBFS), otherwise `textSecondary`
- Show "---" when peak == `kMinDB` (no signal yet)

## Produces
None

## Consumes
None

## Relevant Files
Modify: `M-LIM/src/ui/LevelMeter.cpp` — add peak label rendering in `drawChannel()` and/or `paint()`
Modify: `M-LIM/src/ui/LevelMeter.h` — add `kPeakLabelH` constant if needed
Read: `M-LIM/src/ui/Colours.h` — colour constants

## Acceptance Criteria
- [ ] Run: `cmake --build build --config Release -j$(nproc) && echo OK` → Expected: `OK`
- [ ] Run: build plugin, play audio to trigger peak hold, screenshot — expected: small numeric peak value visible at top of each level meter bar channel
- [ ] Run: `cd build && ctest --output-on-failure` → Expected: all tests pass

## Tests
None (display-only change)

## Technical Details

Add a constant to `LevelMeter.h`:
```cpp
static constexpr float kPeakLabelH = 14.0f;
```

In `LevelMeter::paint()`, after computing `barL` and `barR`, offset them to leave room at the top for the peak label:
```cpp
// Before drawing the bars, remove the peak label area from the top of each bar:
auto peakLabelL = barL.removeFromTop(kPeakLabelH);
auto peakLabelR = barR.removeFromTop(kPeakLabelH);
```

Then in `drawChannel()` (or a new `drawPeakLabel()` helper), render the numeric value:
```cpp
void LevelMeter::drawPeakLabel (juce::Graphics& g,
                                 const juce::Rectangle<float>& labelArea,
                                 float peakDB) const
{
    juce::String text = (peakDB <= kMinDB + 0.5f)
                      ? "---"
                      : juce::String (peakDB, 1);

    juce::Colour col = (peakDB >= kDangerDB) ? MLIMColours::meterDanger
                     : (peakDB >= kWarnDB)   ? MLIMColours::meterWarning
                                             : MLIMColours::textSecondary;
    g.setFont (juce::Font (9.0f));
    g.setColour (col);
    g.drawText (text, labelArea, juce::Justification::centred, false);
}
```

Note: adjusting the bar height by `kPeakLabelH` will shift the existing `dbToNorm` mapping used in `drawChannel()`, so pass the adjusted `bar` (without the peak label strip) to `drawChannel()`.

## Dependencies
None
