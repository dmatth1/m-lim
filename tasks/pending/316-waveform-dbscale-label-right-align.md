# Task 316: Waveform dB Scale Labels — Right-Align After Opacity Reduction

## Description
Active task 306 (waveform-db-labels-reduce-opacity) reduces dB label opacity from 0.75f to 0.35f.
This task handles the complementary alignment fix: in the reference (v1-0010), the dB labels
appear **right-aligned against the left waveform border** rather than extending rightward into
the waveform content.

M-LIM currently draws them **left-aligned** (from `area.getX() + 2.0f`), which causes text to
extend more into the waveform than the reference.

## Fix

In `WaveformDisplay.cpp`, `drawBackground()`, in the dB overlay label block:
1. Change label alignment from `juce::Justification::centredLeft` to `juce::Justification::centredRight`
2. Reduce label rect width from `44.0f` to `38.0f`

Do NOT change the alpha value — that is handled by active task 306.

## Produces
None

## Consumes
None

## Relevant Files
Modify: `M-LIM/src/ui/WaveformDisplay.cpp` — `drawBackground()`: change alignment and width of dB label rects

## Acceptance Criteria
- [ ] Run: `cmake --build M-LIM/build --config Release -j$(nproc) 2>&1 | tail -5` → Expected: build succeeds, exit 0
- [ ] Run: `source Scripts/ui-test-helper.sh && start_app && screenshot screenshots/task-316-after.png && stop_app` → Expected: screenshot saved
- [ ] Visual: dB labels (−3 dB, −6 dB…) right-aligned against the left waveform edge, not extending as far into the waveform

## Tests
None

## Technical Details
In `WaveformDisplay.cpp` `drawBackground()`, find the block drawing dB labels. Change:
```cpp
// BEFORE:
auto labelRect = juce::Rectangle<float> (area.getX() + 2.0f, yPos - 7.0f, 44.0f, 14.0f);
g.drawText (label, labelRect, juce::Justification::centredLeft, false);

// AFTER:
auto labelRect = juce::Rectangle<float> (area.getX() + 2.0f, yPos - 7.0f, 38.0f, 14.0f);
g.drawText (label, labelRect, juce::Justification::centredRight, false);
```

(Exact variable names may differ — search for the justification enum or the label rect construction.)

## Dependencies
Requires active task 306 (waveform-db-labels-reduce-opacity) to complete first
