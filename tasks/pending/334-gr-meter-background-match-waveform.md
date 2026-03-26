# Task 334: GainReductionMeter — Background Matches Waveform Gradient

## Description
The GainReductionMeter component (12px wide strip, kGRMeterW=12) fills its background with
`MLIMColours::displayBackground` (#111118 = near-black). In the reference (prol2-main-ui.jpg),
the narrow meter strip at x≈668-680 shows the SAME steel-blue gradient as the waveform area:

  Reference at x=670, y=100: #7585B4 (steel-blue — waveform gradient color)
  Reference at x=670, y=200: #A3B0CF (lighter steel-blue — gradient at y=200)
  Reference at x=670, y=350: #8087A3 (medium steel-blue)
  M-LIM at x=670, y=100-350: #181818 (dark — displayBackground color)

This dark strip is clearly visible against the waveform and creates an RMSE penalty in the
waveform region. The fix is to paint the GR meter bar area background with the same vertical
gradient as the waveform display, making the GR meter strip visually blend with the waveform.

## Produces
None

## Consumes
None

## Relevant Files
Modify: `M-LIM/src/ui/GainReductionMeter.cpp` — change `paint()` to use waveform gradient instead of displayBackground
Read: `M-LIM/src/ui/Colours.h` — gradient color constants (displayGradientTop, displayGradientBottom)
Read: `M-LIM/src/ui/GainReductionMeter.h` — class structure

## Acceptance Criteria
- [ ] Run: `cmake --build M-LIM/build --config Release -j$(nproc) 2>&1 | tail -3` → Expected: build succeeds, exit 0
- [ ] Run: `source Scripts/ui-test-helper.sh && start_app && screenshot "screenshots/task-334-after.png" && stop_app` → Expected: screenshot saved
- [ ] Visual: The GR meter strip (narrow dark band visible between waveform and level meter) should now blend with the waveform gradient, appearing as an extension of the waveform background rather than a separate dark strip

## Tests
None

## Technical Details
In `M-LIM/src/ui/GainReductionMeter.cpp`, in the `paint()` method, change the bar area
background fill from displayBackground to the waveform gradient:

```cpp
// BEFORE:
// Background
g.setColour (MLIMColours::displayBackground);
g.fillRect (barArea);

// AFTER:
// Background — use waveform gradient to blend with adjacent WaveformDisplay
juce::ColourGradient grBg = juce::ColourGradient::vertical (
    MLIMColours::displayGradientTop,
    barArea.getY(),
    MLIMColours::displayGradientBottom,
    barArea.getBottom());
g.setGradientFill (grBg);
g.fillRect (barArea);
```

Also apply the same gradient to the scale area:
```cpp
// In drawScale(), change any background fill from displayBackground to the gradient
```

Note: The GR meter bar itself (the actual gain reduction indicator drawn by drawBar()) is
drawn on top of the background, so this change only affects the empty background behind the
bar. The GR bar will still be visible against the gradient background.

Also apply the gradient to the `scaleArea` in `paint()` if it is currently filled with
displayBackground. Search for any `g.fillRect` or `g.fillAll` calls in GainReductionMeter.cpp
that use displayBackground and change them to the gradient fill.

## Dependencies
None (can be done in parallel with tasks 328-332)
