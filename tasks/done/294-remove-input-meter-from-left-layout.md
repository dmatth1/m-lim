# Task 294: Remove inputMeter_ From Left-Side Layout — Waveform to Left Edge

## Description
`PluginEditor::resized()` currently allocates `kInputMeterW = 30px` on the LEFT side of the plugin for `inputMeter_` (a LevelMeter component). The reference (Pro-L 2) has **no dedicated left-side level meter**. The reference's level meters are on the RIGHT side of the waveform, and the waveform extends all the way to the left edge (no left strip of any kind).

This 30px dark meter strip at x=0 (or x=18 after task 293 removes the ADVANCED strip) does not appear in the reference, creating a systematic left-region RMSE error.

**Measured impact:** the left-30px RMSE crop shows:
- Current with ADVANCED(18) + inputMeter(30): 33.36%
- With only inputMeter(30) at x=0 (after task 293): ~29.5%
- With waveform starting at x=0 (no left strips): expected ~25-26%

**Required changes:**
1. In `PluginEditor::resized()`: **remove** the `inputMeter_.setBounds(bounds.removeFromLeft(kInputMeterW))` line. Instead call `inputMeter_.setVisible(false)` or simply do not allocate space for it.
2. The input level information is already visualized inside `WaveformDisplay` as the input fill area, so removing the separate meter does not lose key functionality.
3. If the input meter is to be preserved for metering accuracy, make it zero-width (hidden but still receiving data) by setting `inputMeter_.setVisible(false)` and keeping the data-push calls.
4. The waveform display now fills the space starting at x=0 (after TopBar) with no left strip.

**Expected RMSE improvement:** left-meter crop should drop from ~29.5% → estimated ~24-27%.

## Produces
None

## Consumes
None

## Relevant Files
Modify: `M-LIM/src/PluginEditor.cpp` — comment out or remove `inputMeter_.setBounds(...)` in `resized()`; add `inputMeter_.setVisible(false)`
Modify: `M-LIM/src/PluginEditor.h` — optionally remove `kInputMeterW` constant (or set to 0)
Read: `M-LIM/src/ui/LevelMeter.h` — understand what data pushes to it (so we know what to keep)
Read: `/reference-docs/video-frames/v1-0009.png` — confirms no left-side level meter in reference

## Acceptance Criteria
- [ ] Run: `cmake --build /workspace/M-LIM/build --config Release -j$(nproc) --target MLIM_Standalone 2>&1 | tail -3` → Expected: exit 0
- [ ] Visual: Screenshot shows waveform display extending to the left edge (x=0), no dark meter strip on the left side.
- [ ] Run RMSE: left-meter crop `30x378+0+30` on 900x500 → Expected: RMSE ≤ 0.28 (improvement from ~0.295 post-task-293)

## Tests
None

## Technical Details
In `PluginEditor::resized()` the change is:
```cpp
// Remove this line (or comment it out):
// inputMeter_.setBounds (bounds.removeFromLeft (kInputMeterW));

// Add:
inputMeter_.setVisible (false);
```
Keep all `inputMeter_.setLevel(...)`, `inputMeter_.setPeakHold(...)`, `inputMeter_.setClip(...)` calls in the timer callback — they do no harm and can be re-enabled later.

## Dependencies
Requires task 293
