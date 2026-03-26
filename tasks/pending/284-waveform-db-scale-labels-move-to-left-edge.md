# Task 284: Waveform dB Scale Labels — Move to Left Edge

## Description
The dB scale labels overlaid on the waveform are currently drawn on the **right edge** of the waveform
(task 276 placed them there). The Pro-L 2 reference clearly shows these labels on the **left edge**
of the waveform (video frames v1-0009, v1-0010, v1-0011 all confirm this).

Additionally the labels currently alternate between two opacity levels (0.70 / 0.45). The reference
shows uniform opacity for all labels.

Changes required in `WaveformDisplay.cpp` → `drawBackground()`:
1. Reposition the label rect from `area.getRight() - labelW - 4` → `area.getX() + 2`
2. Change text justification from `centredRight` → `centredLeft`
3. Set a single uniform alpha for all labels (e.g. 0.70f) instead of alternating

## Produces
None

## Consumes
None

## Relevant Files
Modify: `M-LIM/src/ui/WaveformDisplay.cpp` — `drawBackground()` lines ~262–281 (the dB overlay
label block added by task 276)
Read: `/reference-docs/video-frames/v1-0009.png` — clearly shows left-side label positioning
Read: `/reference-docs/video-frames/v1-0010.png` — clearly shows left-side label positioning
Read: `/reference-docs/video-frames/v1-0011.png` — clearly shows left-side label positioning

## Acceptance Criteria
- [ ] Build: `cmake --build /workspace/M-LIM/build --config Release -j$(nproc) --target MLIM_Standalone` → Expected: exits 0, no errors
- [ ] Visual: Launch standalone on Xvfb, capture screenshot. Confirm dB labels ("0 dB", "-3 dB", etc.) appear on the LEFT portion of the waveform display, not the right edge.
- [ ] Visual: Confirm all grid labels have the same opacity (no alternating dim/bright pattern).

## Tests
None

## Technical Details
In `drawBackground()` the current label rect is:
```cpp
auto labelRect = juce::Rectangle<float>(
    area.getRight() - labelW - 4.0f,   // RIGHT edge
    y - 6.0f, labelW, 12.0f);
float alpha = (gi % 2 == 0) ? 0.70f : 0.45f;  // alternating
g.drawText(label, labelRect, juce::Justification::centredRight, false);
```
Change to:
```cpp
auto labelRect = juce::Rectangle<float>(
    area.getX() + 2.0f,               // LEFT edge
    y - 6.0f, labelW, 12.0f);
float alpha = 0.60f;                  // uniform
g.drawText(label, labelRect, juce::Justification::centredLeft, false);
```

## Dependencies
None
