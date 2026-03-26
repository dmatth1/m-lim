# Task 261: Waveform Display — Hide Mode Label When Not Hovered

## Description
`WaveformDisplay` renders a display-mode label ("FAST", "SLOW", "SLOWDOWN", "INFINITE", "OFF")
in the top-left corner of the waveform area at **70% opacity** even when the user is not hovering
over it. The FabFilter Pro-L 2 reference shows **no such label** in the waveform area at rest.

This persistent semi-opaque text label adds visual noise that is absent from the reference and
contributes to RMSE divergence in the waveform region.

Fix: make the mode label **fully invisible** (`alpha = 0`) when `modeSelectorHovered_` is false,
and **fully visible** (`alpha = 0.9`) only when the user hovers over the top-left selector region.
This preserves the interactive functionality (hover to reveal, click to cycle, right-click for
menu) while eliminating the visual clutter when idle.

## Produces
None

## Consumes
None

## Relevant Files
Modify: `src/ui/WaveformDisplay.cpp` — `drawModeSelector()`: change the non-hover text colour
  from `textSecondary.withAlpha(0.7f)` to `textSecondary.withAlpha(0.0f)`.
Read: `src/ui/WaveformDisplay.h` — `modeSelectorHovered_` member, `modeSelectorBounds()` method.

## Acceptance Criteria
- [ ] Run: `cd /workspace/M-LIM && cmake --build build -j$(nproc) 2>&1 | tail -3` → Expected: `Built target MLIM_Standalone`
- [ ] Run: `cd /workspace/M-LIM/build && ctest --output-on-failure` → Expected: all tests pass
- [ ] Visual: Launch standalone on Xvfb, take idle screenshot → no mode label text visible in top-left of waveform area

## Tests
None

## Technical Details
Single-line change in `WaveformDisplay.cpp` `drawModeSelector()`:

```cpp
// Before:
juce::Colour textCol = modeSelectorHovered_
    ? MLIMColours::textPrimary.withAlpha (0.9f)
    : MLIMColours::textSecondary.withAlpha (0.7f);

// After:
juce::Colour textCol = modeSelectorHovered_
    ? MLIMColours::textPrimary.withAlpha (0.9f)
    : MLIMColours::textSecondary.withAlpha (0.0f);
```

The hover background pill (`if (modeSelectorHovered_)` guard) already handles hiding the pill
when not hovered — no additional change needed there.

## Dependencies
None
