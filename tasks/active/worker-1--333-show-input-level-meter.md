# Task 333: Show Input Level Meter (Hidden Since Early Task)

## Description
The input level meter (`inputMeter_`) has been hidden via `inputMeter_.setVisible(false)` in
`PluginEditor.cpp`. The `kInputMeterW = 30` constant defines a 30px wide strip for it. In the
reference Pro-L 2, a vertical stereo input level meter is clearly visible on the LEFT side of
the waveform display.

The "Left level meters (30x378 at x=0, y=30)" sub-region shows 23.71% RMSE. Currently, this
region shows the left edge of the waveform display background + dB labels. The reference shows
a stereo vertical meter bar structure. Showing the input meter improves structural match even
at rest (no audio) because the meter bar shape, track background, and scale labels will align
with the reference layout.

**Changes needed:**
1. Remove `inputMeter_.setVisible(false)` in `PluginEditor.cpp`
2. Re-enable `bounds.removeFromLeft(kInputMeterW)` in `resized()` before setting waveform bounds

## Produces
None

## Consumes
None

## Relevant Files
Modify: `src/PluginEditor.cpp` — remove setVisible(false), add removeFromLeft in resized()

## Acceptance Criteria
- [ ] Run: build and screenshot → Expected: 30px input meter bar visible on left edge
- [ ] Run: `cd /workspace/M-LIM && cmake --build build -j$(nproc) 2>&1 | grep -c "error:"` → Expected: `0`
- [ ] Visual: input meter is a narrow 30px stereo bar meter on the left side of the waveform display

## Tests
None

## Technical Details
In `src/PluginEditor.cpp`, in the `resized()` method, change:

```cpp
// BEFORE:
// Input level meter hidden — waveform extends to left edge (no left strip)
inputMeter_.setVisible (false);
```

To:

```cpp
// Input level meter: left strip (matches Pro-L 2 reference layout)
inputMeter_.setBounds (bounds.removeFromLeft (kInputMeterW));
```

The `inputMeter_` remains `addAndMakeVisible` in the constructor, just re-enable its bounds by
removing the `setVisible(false)` call and adding `setBounds`.

Note: the waveform display will narrow by 30px (from ~668px to ~638px wide). This is correct —
the waveform area in the reference is narrower than the full plugin width.

## Dependencies
None
