# Task 293: Fix ADVANCED Button — Move From Left-Edge Full-Height Strip to Right Side of Control Strip

## Description
Task 292 placed the ADVANCED button as an 18px full-height vertical strip on the far LEFT of the plugin content (spanning from below the TopBar all the way to the bottom of the ControlStrip). This was based on a misreading of the reference. The reference (v1-0009) shows ADVANCED as a **narrow vertical strip on the RIGHT side of the control strip** — between the CHANNEL LINKING knobs and the level meter panel — NOT a full-height strip on the left.

This is the primary cause of the left-level-meter RMSE regression from 29.46% → 33.36% (+3.9pp) after task 292. The 18px dark strip at x=0 of the plugin does not match any reference content at that position; the reference shows waveform content starting at the left edge.

**Required changes:**
1. In `PluginEditor::resized()`: **remove** the `bounds.removeFromLeft(kAdvancedBtnW)` block. Do NOT allocate left-edge space for the ADVANCED button.
2. The ADVANCED button now lives inside the **ControlStrip** component. The ControlStrip's `resized()` should allocate a narrow strip on the **right side** of its knob row (before the right panel / OUTPUT slider region), approximately 18px wide, for the ADVANCED button.
3. Move the `advancedButton_` member (and its wiring) back to `ControlStrip.h/.cpp` (it was there before task 292 removed it). OR: keep advancedButton_ in PluginEditor but position it via `setBounds` to a rect within the control strip right side.
4. Remove the `paint()` override code in `PluginEditor::paint()` that draws the rotated ADVANCED text/background (move it to `ControlStrip::paint()` or render it within the control strip).
5. The `kAdvancedBtnW = 18` constant in `PluginEditor.h` can be removed or repurposed; the waveform+inputMeter layout no longer subtracts it from the left.

**Expected RMSE improvement:** left-meter RMSE should recover from 33.36% → ~29.46% (pre-task-292 level).

## Produces
None

## Consumes
None

## Relevant Files
Modify: `M-LIM/src/PluginEditor.h` — remove or repurpose `kAdvancedBtnW`; adjust or remove advancedButton_ member if moved back to ControlStrip
Modify: `M-LIM/src/PluginEditor.cpp` — remove `advancedButton_.setBounds(bounds.removeFromLeft(kAdvancedBtnW))` from `resized()`; remove ADVANCED paint code from `paint()`
Modify: `M-LIM/src/ui/ControlStrip.h` — re-add `advancedButton_` member (or accept callback from PluginEditor), `isAdvancedExpanded_` toggle state
Modify: `M-LIM/src/ui/ControlStrip.cpp` — add ADVANCED strip to right side of knob row in `resized()`; add rotated-text drawing in `paint()`
Read: `/reference-docs/video-frames/v1-0009.png` — ADVANCED appears on the right side of the control strip

## Acceptance Criteria
- [ ] Run: `cmake --build /workspace/M-LIM/build --config Release -j$(nproc) --target MLIM_Standalone 2>&1 | tail -3` → Expected: build succeeds (exit 0)
- [ ] Visual: Launch on Xvfb, screenshot. Left edge of plugin (x=0) shows waveform content (blue-grey background), NOT a dark ADVANCED strip. The ADVANCED label appears within the control strip on the right side.
- [ ] Run RMSE: left-meter crop `30x378+0+30` on 900x500 → Expected: RMSE ≤ 0.31 (improvement from current 0.3336)

## Tests
None

## Technical Details
Reference layout of the control strip from right-to-left: OUTPUT slider → level meter small panel → ADVANCED strip (vertical, ~18px) → CHANNEL LINKING (TRANSIENTS + RELEASE) → RELEASE → ATTACK → LOOKAHEAD → STYLE selector.

The ADVANCED text is rotated 90° counter-clockwise as a vertical label in this strip.

## Dependencies
None
