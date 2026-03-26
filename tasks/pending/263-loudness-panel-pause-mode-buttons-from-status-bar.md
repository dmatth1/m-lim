# Task 263: Loudness Panel — Move Pause/Mode Buttons Out of Status Bar

## Description
In the FabFilter Pro-L 2 reference (`prol2-main-ui.jpg`), the **"||" pause button** and
**"Short Term" mode button** appear inside the loudness/meter right panel area, directly below
the large LUFS numeric readout — not in the bottom status bar.

In M-LIM these two buttons (`pauseMeasurementButton_` and `measurementModeButton_`) are
currently placed in the `ControlStrip` status bar row (far right). This clutters the status
bar and mismatches the reference layout.

**Required change:**
1. Remove `pauseMeasurementButton_` and `measurementModeButton_` from `ControlStrip`'s
   status bar setup, `addAndMakeVisible`, and `resized()`.
2. Add them to `LoudnessPanel` — below the large LUFS readout, above the "Out" readout.
3. Preserve existing `onClick` lambdas / APVTS wiring so functionality is unchanged.

## Produces
None

## Consumes
None

## Relevant Files
Modify: `src/ui/ControlStrip.cpp` — remove `pauseMeasurementButton_` and
  `measurementModeButton_` from status-bar setup and `resized()`.
Modify: `src/ui/ControlStrip.h` — move button members to `LoudnessPanel` or keep as members
  and pass refs; either approach acceptable.
Modify: `src/ui/LoudnessPanel.cpp` — add layout region below large LUFS readout for the two
  buttons; `addAndMakeVisible` and `setBounds` in `resized()`.
Modify: `src/ui/LoudnessPanel.h` — add button members or accept refs.
Modify: `src/PluginEditor.cpp` / `src/PluginEditor.h` — update wiring if button ownership moves.
Read: `/reference-docs/reference-screenshots/prol2-main-ui.jpg` — confirm exact visual layout.

## Acceptance Criteria
- [ ] Run: `cd /workspace/M-LIM && cmake --build build -j$(nproc) 2>&1 | tail -3` → Expected: `Built target MLIM_Standalone`
- [ ] Run: `cd /workspace/M-LIM/build && ctest --output-on-failure` → Expected: all tests pass
- [ ] Visual: Screenshot → status bar row no longer contains `||` or `Short Term` buttons
- [ ] Visual: Screenshot → right panel shows `||` and `Short Term` buttons below LUFS readout

## Tests
None

## Technical Details
Simplest approach: move button member variables and their `addAndMakeVisible` + `setBounds`
into `LoudnessPanel`. The `onClick` lambdas and any APVTS attachment must follow.

If `ControlStrip` still needs access to the buttons (e.g., for wiring `onClick` lambdas),
expose them via public getters in `LoudnessPanel` and wire from `PluginEditor`.

In `LoudnessPanel::resized()`, carve out a ~24 px tall row immediately below the large LUFS
readout for the two small buttons (e.g., `[||]` and `[Short Term]` side by side).

## Dependencies
None
