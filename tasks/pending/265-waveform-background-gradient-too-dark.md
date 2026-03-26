# Task 265: Waveform Display Background Gradient Too Dark

## Description
The waveform display background gradient is approximately 35% too dark compared to the
reference. This creates a dull, muted appearance in the idle (no-signal) state and also
affects how waveform overlays composite against the background when audio is playing.

**Measured reference values (from /reference-docs/video-frames/v1-0005.png):**
- Top of waveform display: `#8992AB` (RGB 137, 146, 171)
- Middle area: `#6F7790` (RGB 111, 119, 144)
- These match the comment in `Colours.h` which says "Reference samples from Pro-L 2: top
  ~#8992AB, middle ~#6F7790 (measured from v1-0005.png)" — but the actual constants were
  later changed to darker values.

**Current M-LIM values (too dark):**
- `displayGradientTop = 0xff5A6075` → renders as ~`#585E73`
- `displayGradientBottom = 0xff3E4258` → renders as ~`#3C4056`

**Current grid line issue:**
- `waveformGridLine = 0xff474D62` was tuned against the old darker background.
- With the corrected brighter background, this colour will appear as harsh dark stripes.
- It must be updated to maintain the same subtle −19 unit relative darkness.

**Fix — update three constants in `src/ui/Colours.h`:**

```cpp
// Change from:
const juce::Colour displayGradientTop   { 0xff5A6075 };
const juce::Colour displayGradientBottom{ 0xff3E4258 };
const juce::Colour waveformGridLine     { 0xff474D62 };

// Change to:
const juce::Colour displayGradientTop   { 0xff8892AA };  // matches reference top ~#8992AB
const juce::Colour displayGradientBottom{ 0xff606878 };  // matches reference middle ~#6F7790
const juce::Colour waveformGridLine     { 0xff757F97 };  // −19 units vs new top (same relative contrast)
```

Update the comment on `displayGradientTop` to reflect the corrected values.

## Produces
None

## Consumes
None

## Relevant Files
Modify: `src/ui/Colours.h` — update `displayGradientTop`, `displayGradientBottom`,
  `waveformGridLine`
Read:   `src/ui/WaveformDisplay.cpp` — verify gradient is used in `drawBackground()`
Read:   `/reference-docs/video-frames/v1-0005.png` — reference waveform background colour

## Acceptance Criteria
- [ ] Run: `cd /workspace/M-LIM && cmake --build build -j$(nproc) 2>&1 | tail -3` →
      Expected: build succeeds with no errors
- [ ] Run: `cd /workspace/M-LIM/build && ctest --output-on-failure` →
      Expected: all tests pass
- [ ] Visual check: launch standalone on Xvfb, take screenshot, verify waveform
      display background matches the medium blue-gray in `v1-0005.png` (not the previous
      darker/dimmer appearance)
- [ ] Run: `source Scripts/ui-test-helper.sh && start_app && screenshot /tmp/after.png &&
      compare -metric RMSE /tmp/after-crop.png /tmp/ref-crop.png /dev/null 2>&1` →
      Expected: RMSE lower than the 25.4% baseline recorded in
      `screenshots/task-264-rmse-results.txt`

## Tests
None (visual rendering change — no unit tests required)

## Technical Details
Only `Colours.h` needs to change. No logic changes required. The three constants are:

1. `displayGradientTop`: the colour at the very top of the waveform display. Target `0xff8892AA`.
2. `displayGradientBottom`: the colour at the bottom edge. Target `0xff606878`.
3. `waveformGridLine`: the horizontal dB grid lines drawn inside the waveform. Must stay
   visually subtle. Target `0xff757F97` (approximately 19 units darker than the new top
   gradient, same relative contrast ratio as before).

Do NOT change `displayBackground` (used for the left dB scale strip fill and off-state
fill) — it should remain `0xff111118`.

Do NOT change any waveform overlay colours (`inputWaveform`, `outputWaveform`,
`gainReduction`, `outputEnvelope`) — these are composited on top and will look correct
against the new background.

## Dependencies
None
