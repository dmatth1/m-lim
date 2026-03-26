# Task 267: Waveform Display — Remove Internal Left dB Scale to Eliminate Double-Scale Artifact

## Description
The waveform display reserves a 30 px strip on its LEFT edge (`kScaleWidth = 30`) for dB
scale labels, drawn by `WaveformDisplay::drawScale()`. The `inputMeter_` component (48 px
wide) sits immediately to the left of the waveform display and draws its OWN dB scale on
its right edge (18 px). This creates two adjacent columns of dB numbers visible to the
user, which looks incorrect and contributes to the high RMSE (29%) measured for the left
side of the plugin.

**Layout in PluginEditor (from PluginEditor.h constants):**
```
x=0..48:   inputMeter_  (LevelMeter, draws scale at x=30..48 on its right edge)
x=48..78:  WaveformDisplay's internal kScaleWidth=30 scale strip
x=78..672: Actual waveform content area
```

The result is two sets of dB labels from x=30 to x=78 — a double-scale visual artifact
not present in the reference Pro-L 2.

**In the reference (v1-0020.png):** The waveform display fills the space without a
separate scale strip at the left edge. The gain-reduction dB context is provided by grid
lines inside the waveform area, not a separate labeled strip.

**Fix:**
In `src/ui/WaveformDisplay.h`, change:
```cpp
static constexpr float kScaleWidth = 30.0f;
```
to:
```cpp
static constexpr float kScaleWidth = 0.0f;
```

With `kScaleWidth = 0`, the `scaleArea` in `paint()` will be empty (zero width),
`drawScale()` will draw nothing, and `displayArea` will cover the full waveform component
bounds. The horizontal dB grid lines in `drawBackground()` already span the full
`displayArea` and will remain visible throughout the waveform as subtle context
markers. The `inputMeter_`'s own scale on its right edge continues to provide dB labels.

No other code changes are needed — `scaleArea.isEmpty()` with width=0 safely no-ops
`fillRect`, `drawVerticalLine`, and `drawText` for all calls in `drawScale()`.

## Produces
None

## Consumes
None

## Relevant Files
Modify: `src/ui/WaveformDisplay.h` — change `kScaleWidth` from 30.0f to 0.0f
Read:   `src/ui/WaveformDisplay.cpp` — verify `paint()` and `drawScale()` handle zero
  scaleArea correctly (all drawing calls on empty rect are no-ops in JUCE)
Read:   `src/ui/LevelMeter.cpp` — understand existing scale drawn by adjacent inputMeter_
Read:   `src/PluginEditor.h` — confirm `kInputMeterW = 48` is still present

## Acceptance Criteria
- [ ] Run: `cd /workspace/M-LIM && cmake --build build -j$(nproc) 2>&1 | tail -3` →
      Expected: build succeeds with no errors
- [ ] Run: `cd /workspace/M-LIM/build && ctest --output-on-failure` →
      Expected: all tests pass
- [ ] Visual check: launch standalone on Xvfb, take screenshot — only ONE column of dB
      labels should be visible at the left of the waveform area (from the inputMeter_
      scale), NOT two adjacent columns

## Tests
None (layout constant change — no unit tests required)

## Technical Details
`WaveformDisplay::paint()` currently:
```cpp
auto scaleArea = bounds.removeFromLeft (kScaleWidth);  // 30 px
auto displayArea = bounds;                              // remainder
drawBackground (g, displayArea);
...
drawScale (g, scaleArea);
```

With `kScaleWidth = 0.0f`, `scaleArea` will be an empty rectangle. JUCE rectangle
operations (`fillRect`, `drawText`, etc.) on zero-width rectangles are safe no-ops, so no
guard is needed. `displayArea` will equal the full `bounds`, giving the waveform content
30 more pixels of width.

The ceiling line draw call also uses `scaleArea`:
```cpp
drawCeilingLine (g, displayArea, scaleArea);
```
`drawCeilingLine` uses `scaleArea` only to draw the small `"+0.0 dB"` ceiling label:
```cpp
auto labelRect = juce::Rectangle<float> (scaleArea.getX() + 2.0f,
                                          y - 6.0f,
                                          scaleArea.getWidth() - 4.0f,   // = -4 when kScaleWidth=0
                                          12.0f);
g.drawText (label, labelRect, ...);
```
With `kScaleWidth = 0`, `scaleArea.getWidth() - 4.0f = -4`, giving a negative-width
rectangle. JUCE's `drawText()` is a no-op on invalid rectangles, so this is safe — the
small ceiling label simply disappears, which is the correct behaviour (there is no scale
strip left to draw it in). The ceiling LINE itself is drawn using `area` bounds and is
unaffected.

## Dependencies
None
