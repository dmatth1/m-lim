# Task 292: ADVANCED Button — Move to Left Edge of Plugin (Full-Height Vertical Strip)

## Description
In the Pro-L 2 reference (visible in `v1-0009.png`), the "ADVANCED" toggle is a narrow full-height
vertical strip on the **far left** of the entire plugin window, spanning from the waveform area down
through the control strip. M-LIM currently places the ADVANCED button only inside the control strip
(rightmost element of the knob row).

Moving ADVANCED to the left edge matches the reference layout and frees up space in the control
strip knob row.

**Implementation approach:**
1. Remove `advancedButton_` from `ControlStrip` and its knob-row layout
2. Add a new `juce::TextButton advancedButton_` to `PluginEditor` as a full-height strip on the
   left edge of the plugin content area (between TopBar and bottom of ControlStrip), positioned at
   x=0 with width ~18px
3. Keep the rotated "ADVANCED" text rendering logic (from `ControlStrip::paint()`) but move it to
   `PluginEditor` paint or a thin helper component
4. The existing `isAdvancedExpanded_` state and `advancedButton_.onClick` wiring stays the same
5. Adjust `kInputMeterW` positioning in `PluginEditor::resized()` to account for the 18px strip on
   the left

## Produces
None

## Consumes
None

## Relevant Files
Modify: `M-LIM/src/PluginEditor.h` — add advancedButton_ member, adjust layout constants
Modify: `M-LIM/src/PluginEditor.cpp` — add button to left edge in resized(), wire toggle callback
Modify: `M-LIM/src/ui/ControlStrip.h` — remove advancedButton_ and isAdvancedExpanded_ members
Modify: `M-LIM/src/ui/ControlStrip.cpp` — remove ADVANCED button setup, layout, and paint code
Read: `/reference-docs/video-frames/v1-0009.png` — shows ADVANCED as full-height left strip

## Acceptance Criteria
- [ ] Build: `cmake --build /workspace/M-LIM/build --config Release -j$(nproc) --target MLIM_Standalone` → Expected: exits 0
- [ ] Visual: Screenshot shows a narrow (~18px) vertical strip on the far left of the plugin
  content area with rotated "ADVANCED" text, spanning from waveform to control strip bottom.
- [ ] Behaviour: Clicking the strip toggles advanced mode (no regression to existing toggle logic).
- [ ] Control strip knob row has no ADVANCED button slot; knobs use the freed space.

## Tests
None

## Technical Details
- The ADVANCED strip should be `kAdvancedBtnW = 18` px wide
- In `PluginEditor::resized()`, remove 18px from the left of the content bounds before placing
  `inputMeter_` and `waveformDisplay_`, e.g.:
  ```cpp
  auto advancedStrip = bounds.removeFromLeft(kAdvancedBtnW);
  advancedButton_.setBounds(advancedStrip);
  ```
- The rotated-text paint logic (currently in `ControlStrip::paint()`) moves to a custom `paint()`
  override on a thin component or directly in `PluginEditor::paint()` using the same
  `g.addTransform(AffineTransform::rotation(...))` pattern already in ControlStrip.

## Dependencies
None
