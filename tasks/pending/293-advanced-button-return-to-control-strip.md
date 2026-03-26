# Task 293: ADVANCED Button — Return to ControlStrip (Fix Task 292 RMSE Regression)

## Description
Task 292 moved the ADVANCED button from ControlStrip to the far-left edge of the plugin (18px wide
vertical strip at x=0). This was based on a misinterpretation of the video frame. The main reference
screenshot `prol2-main-ui.jpg` clearly shows ADVANCED as a narrow vertical tab **inside the control
strip**, positioned between the CHANNEL LINKING section and the OUTPUT slider — NOT at the plugin's
far left edge.

This misplacement caused a **+3.90 pp RMSE regression** on the left 30px meter strip (29.46% →
33.36%) because x=0-18 now shows the dark `algoButtonInactive` (#303848) rounded rectangle instead
of what the reference shows at those positions (waveform content / level meter bars). The full-image
RMSE also regressed ~0.2 pp.

**Fix**:
1. Remove `advancedButton_` from `PluginEditor`: remove the member, `kAdvancedBtnW` constant,
   `addAndMakeVisible`, `onClick` wiring, `setBounds` call in `resized()`, and the paint block.
2. Add `advancedButton_` back to `ControlStrip.h` as a member.
3. In `ControlStrip::resized()`, allocate a narrow slot (12px wide) for ADVANCED between the
   `channelLinkReleaseKnob_` slot and the `outputCeilingSlider_` right column.
4. In `ControlStrip::paint()`, draw the ADVANCED button as a rotated-text tab (re-use the same
   rotated-text rendering style that was already in ControlStrip before task 292).
5. The `isAdvancedExpanded_` state and its `onClick` logic should live in ControlStrip and wire
   back up to PluginEditor via a callback (or keep it simple: ControlStrip owns the state and
   hides/shows the CHANNEL LINKING knobs itself).

After this fix, the left layout from x=0 will be:
- `inputMeter_` at x=0 (30px wide) — no ADVANCED strip occupying x=0-18

## Produces
None

## Consumes
None

## Relevant Files
Modify: `M-LIM/src/PluginEditor.h` — remove `advancedButton_`, `isAdvancedExpanded_`, `kAdvancedBtnW`
Modify: `M-LIM/src/PluginEditor.cpp` — remove all ADVANCED button setup, paint(), resized() code
Modify: `M-LIM/src/ui/ControlStrip.h` — add `advancedButton_` member back, add `isAdvancedExpanded_`
Modify: `M-LIM/src/ui/ControlStrip.cpp` — add ADVANCED tab in resized() between CL-release and output slider; add paint() rotated text for it
Read: `/reference-docs/reference-screenshots/prol2-main-ui.jpg` — control strip bottom-right shows ADVANCED tab position
Read: `/workspace/screenshots/ref-control.png` — cropped reference control strip (already saved by auditor)

## Acceptance Criteria
- [ ] Run: `cmake --build /workspace/M-LIM/build --config Release -j$(nproc) --target MLIM_Standalone` → Expected: exits 0
- [ ] Visual: Screenshot shows NO dark strip at x=0 left edge of plugin — the leftmost element is the input level meter starting at x=0
- [ ] Visual: Control strip shows an "ADVANCED" rotated-text tab between the channel-linking release knob and the output ceiling slider
- [ ] Run: `source /workspace/Scripts/ui-test-helper.sh && start_app && screenshot "task-293-after.png" && stop_app` then `compare -metric RMSE /workspace/screenshots/left-meter-ref.png <(convert /workspace/screenshots/task-293-after.png -crop 30x378+0+30 png:-) /tmp/left-diff.png 2>&1` → Expected: RMSE < 0.32 (improvement from current 0.3336)

## Tests
None

## Technical Details
The ADVANCED button in the reference (visible in `ref-control.png` saved at
`/workspace/screenshots/ref-control.png`) is at approximately x=535-550 in the 900px wide control
strip image — a narrow vertical tab with "ADVANCED" text rotated 90°, right-aligned before the
output section.

In `ControlStrip::resized()`, current knob slot allocation:
```cpp
int knobW = knobRow.getWidth() / 7;
algorithmSelector_.setBounds (knobRow.removeFromLeft (knobW * 2));
lookaheadKnob_.setBounds     (knobRow.removeFromLeft (knobW));
attackKnob_.setBounds        (knobRow.removeFromLeft (knobW));
releaseKnob_.setBounds       (knobRow.removeFromLeft (knobW));
channelLinkTransientsKnob_.setBounds (knobRow.removeFromLeft (knobW));
channelLinkReleaseKnob_.setBounds    (knobRow);
```

Change to reserve 12px for ADVANCED before channelLinkRelease fills the remainder:
```cpp
int knobW = (knobRow.getWidth() - 12) / 7;
// ... same assignments ...
channelLinkTransientsKnob_.setBounds (knobRow.removeFromLeft (knobW));
auto advancedSlot = knobRow.removeFromLeft (12);
advancedButton_.setBounds (advancedSlot);
channelLinkReleaseKnob_.setBounds (knobRow);
```

Paint the ADVANCED tab with rotated text in `ControlStrip::paint()` (or override via a child
component). Re-use the existing rotation pattern from PluginEditor.cpp lines 176-200.

In PluginEditor::resized(), remove `advancedButton_.setBounds(bounds.removeFromLeft(kAdvancedBtnW))` — the `removeFromLeft` call is shifting inputMeter_ 18px to the right, which is the root cause of the regression.

## Dependencies
None
