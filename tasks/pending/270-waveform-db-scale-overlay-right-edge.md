# Task 270: Waveform dB Scale Labels — Overlay on Right Edge (Pro-L 2 Style)

## Description
In Pro-L 2, the dB scale labels ("0 dB", "-3 dB", "-6 dB", ..., "-30 dB") appear as
**overlay text inside the waveform display area, right-aligned near its right edge**
(visible in reference frames v1-0003, v1-0008, v1-0009, v1-0016).

Currently M-LIM:
- `kScaleWidth = 0.0f` in WaveformDisplay — no internal scale strip (removed in task 267)
- Scale labels are rendered by the LevelMeter component to the **left** of the waveform
- `kScaleWidth = 18` in LevelMeter.cpp (right-edge label strip)

Required changes:
1. **LevelMeter.cpp**: Remove the `drawScale()` call and set `kScaleWidth = 0` — the meter
   bars should fill the full width without a right-side label strip.
2. **PluginEditor.h**: Reduce `kInputMeterW` from 48 → 30 (just the bar area, no label strip).
3. **WaveformDisplay.cpp / drawBackground()**: After drawing the gradient and grid lines,
   render dB overlay text right-aligned in the last 42 px of the waveform content area:
   - Labels: `"0 dB"` (at 0 dBFS) and `"-3 dB"`, `"-6 dB"`, ..., `"-27 dB"` for each grid line
   - Y position: same as each corresponding horizontal grid line
   - Color: `MLIMColours::textSecondary.withAlpha(0.70f)` — semi-transparent, subtle
   - Font: `MLIMColours::kFontSizeSmall` (9pt)
   - Justify: `Justification::centredRight` within a 40px-wide rect at the right interior edge
   - Leave a 4 px margin from the right edge of the waveform area (before the GR meter border)

Reference evidence: See `v1-0003-right.png` crop — "0 dB", "-3 dB", "-6 dB", ..., "-30 dB"
labels appear at the right edge of the waveform, inside the content area.

## Produces
None

## Consumes
None

## Relevant Files
Modify: `src/ui/WaveformDisplay.cpp` — add overlay text in `drawBackground()` after grid lines
Modify: `src/ui/WaveformDisplay.h` — no constant changes needed (kScaleWidth stays 0)
Modify: `src/ui/LevelMeter.cpp` — remove `drawScale()` call from `paint()`, set kScaleWidth=0
Modify: `src/PluginEditor.h` — reduce `kInputMeterW` from 48 → 30
Read: `src/ui/Colours.h` — colour constants (textSecondary, kFontSizeSmall)
Skip: `src/dsp/` — not relevant

## Acceptance Criteria
- [ ] Run: `cmake --build build -j$(nproc) 2>&1 | tail -3` → Expected: build succeeds, exit 0
- [ ] Run: `cd build && ctest --output-on-failure 2>&1 | tail -3` → Expected: all tests pass
- [ ] Run: `source Scripts/ui-test-helper.sh && start_app && screenshot /tmp/task-270-after.png && stop_app` → Expected: screenshot shows dB labels ("0 dB", "-3 dB" etc.) as overlay text at the RIGHT edge of the waveform display area
- [ ] Run: `convert /tmp/task-270-after.png -crop 60x350+820+35 +repage /tmp/t270-right.png && identify /tmp/t270-right.png` → Expected: crop exists (confirms dB label region at right of waveform)
- [ ] Visual check: Left LevelMeter bars fill their width with no right-side numeric labels; dB numbers appear only inside the waveform right edge

## Tests
None

## Technical Details
In `WaveformDisplay::drawBackground()`, after the grid line loop, add:
```cpp
// dB overlay labels — right-aligned on the waveform right edge (Pro-L 2 style)
g.setFont (juce::Font (MLIMColours::kFontSizeSmall));
for (int gi = 0; gi < kWaveformGridDBCount; ++gi)
{
    const float db  = MLIMColours::kMeterGridDB[gi];
    float frac      = (-db) / kMaxGRdB;
    float y         = area.getY() + frac * area.getHeight();
    if (y < area.getY() || y > area.getBottom()) continue;

    juce::String label = (db == 0.0f) ? "0 dB"
                                      : juce::String (juce::roundToInt (db)) + " dB";
    const float labelW = 40.0f;
    auto labelRect = juce::Rectangle<float> (
        area.getRight() - labelW - 4.0f,
        y - 6.0f,
        labelW,
        12.0f);
    // Alternate alpha so labels don't crowd the waveform content
    float alpha = (gi % 2 == 0) ? 0.70f : 0.45f;
    g.setColour (MLIMColours::textSecondary.withAlpha (alpha));
    g.drawText (label, labelRect, juce::Justification::centredRight, false);
}
```

In `LevelMeter.cpp`, change `kScaleWidth` from 18 → 0 and remove the call to
`drawScale(g, barL.getY(), barL.getHeight())` from `paint()` (or make `drawScale` a no-op
when kScaleWidth==0). Also update the `availW` calculation to not subtract kScaleWidth.

In `PluginEditor.h`, change `kInputMeterW` from 48 → 30.

Take screenshots using the ui-test-helper.sh before and after:
```bash
source Scripts/ui-test-helper.sh
start_app
screenshot screenshots/task-270-before.png
# make code changes, rebuild, restart app
screenshot screenshots/task-270-after.png
compare_to_reference /reference-docs/reference-screenshots/prol2-main-ui.jpg \
  screenshots/task-270-after.png 0.15
stop_app
```

## Dependencies
None
