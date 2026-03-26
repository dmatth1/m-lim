# Task 258: Waveform Display — Hide Mode Label ("FAST") When Not Hovered

## Description
The `WaveformDisplay` renders a display-mode label ("FAST", "SLOW", "SLOWDOWN", "INFINITE", "OFF")
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
Modify: `src/ui/WaveformDisplay.cpp` — `drawModeSelector()` method (line ~221):
  Change the non-hover text colour from `textSecondary.withAlpha(0.7f)` to
  `textSecondary.withAlpha(0.0f)` (fully transparent). The hover branch stays at `0.9f`.

Read: `src/ui/WaveformDisplay.h` — `modeSelectorHovered_` member, `modeSelectorBounds()` method.

## Acceptance Criteria
- [ ] Build: `cd /workspace/M-LIM && cmake --build build -j$(nproc)` → Expected: exit 0
- [ ] Visual: Launch standalone on Xvfb; take screenshot of idle state → **no "FAST" text visible**
  in the top-left corner of the waveform area
- [ ] Visual: Move mouse over the top-left of the waveform → "FAST" (or current mode name) becomes
  visible; move mouse away → label disappears again
- [ ] No regression: clicking the mode selector area still cycles through modes; right-click still
  shows popup menu

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

Also suppress the hover background pill when alpha is 0 (the `if (modeSelectorHovered_)` guard
already handles this, so no additional change needed there).

## Dependencies
None
