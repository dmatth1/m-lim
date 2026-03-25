# Task 195: ControlStrip — "STYLE" and "CHANNEL LINKING" Labels Drawn Outside Component Bounds

## Description
`ControlStrip::paint()` draws a "STYLE" label above the `AlgorithmSelector` and, when the Advanced panel is open, a "CHANNEL LINKING" label above the linking knobs. Both are positioned using `getBounds().getY() - 12`, which evaluates to a **negative Y coordinate** in the component's local coordinate system — placing the text above the component boundary where JUCE's paint clipper cuts it off. Neither label is ever visible.

Root cause: the knob row starts at `kPadding = 4px` from the top of the ControlStrip, but the labels need 12px above the knob row. `4 - 12 = -8`, which is outside the [0, height] clip region.

The reference Pro-L 2 (`prol2-features.jpg`, `prol2-main-ui.jpg`) shows:
- A "STYLE" label clearly visible above the algorithm dropdown selector
- A "CHANNEL LINKING" group label above the transients/release knobs when expanded

Fix: The knob row must start low enough to leave room for the labels. Increase the top inset from `kPadding` (4 px) to `kPadding + kStyleLabelH` (4 + 12 = 16 px) so the labels at `knobRow.getY() - 12` land at `y = 4` (inside the component). Adjust total layout arithmetic accordingly.

The RotaryKnob children (`lookaheadKnob_`, etc.) already handle their own LOOKAHEAD/ATTACK/RELEASE labels inside their own component bounds — only the externally painted `STYLE` and `CHANNEL LINKING` labels are affected.

## Produces
None

## Consumes
None

## Relevant Files
Modify: `M-LIM/src/ui/ControlStrip.cpp` — in `resized()`, shift knob row down so labels fit above it; in `paint()`, confirm label y-coordinates are inside bounds
Read: `M-LIM/src/ui/ControlStrip.h` — layout constants (kKnobRowH=70, kPadding=4, kBtnRowH=24, kControlStripH=120)
Read: `/reference-docs/reference-screenshots/prol2-features.jpg` — shows "STYLE" label above algorithm dropdown and "CHANNEL LINKING" label visible above linking knobs
Read: `/reference-docs/reference-screenshots/prol2-main-ui.jpg` — full UI layout reference

## Acceptance Criteria
- [ ] Run: `cmake --build build --config Release -j$(nproc) && echo OK` → Expected: `OK`
- [ ] Run: `source Scripts/ui-test-helper.sh && start_app && screenshot "screenshots/task-195-after.png" && stop_app` → Expected: "STYLE" text visible above algorithm dropdown; no overflow or clipping artifacts
- [ ] Run: `cd build && ctest --output-on-failure` → Expected: all tests pass

## Tests
None (layout-only change)

## Technical Details
In `ControlStrip::resized()` (`ControlStrip.cpp`):

Introduce a constant for the label height above the knob row:
```cpp
static constexpr int kKnobLabelH = 12;  // pixels above knob row for section labels
```

Change the top inset when slicing the knob row from `bounds.getY()` to account for the label:
```cpp
// In resized():
// Reserve label-height headroom at the top of the knob area
auto bounds = getLocalBounds().reduced(kPadding);
bounds.removeFromTop(kKnobLabelH);          // <-- NEW: headroom for "STYLE" label
auto knobRow = bounds.removeFromTop(kKnobRowH);
```

After this change, `knobRow.getY()` will be `kPadding + kKnobLabelH = 16` in component coordinates. Then in `paint()`:
```cpp
algoB.getY() - 12  →  16 - 12 = 4  ✓  (inside component)
```

Similarly for the CHANNEL LINKING label — after the change `linkBounds.getY() - 12` will be inside bounds.

The remaining 4px reduction from the knob row height is acceptable: 120 - 2×4(padding) - 12(label) - 70(knobs) - 4(separator gap) - 24(status bar) = 2px margin. If space is too tight, consider reducing `kKnobRowH` from 70 to 68.

Also check: the separator line drawn at `kKnobRowH + kPadding` in `paint()` must be updated to `kKnobLabelH + kKnobRowH + kPadding` to keep it correctly positioned between the knob and status rows.

## Dependencies
None
