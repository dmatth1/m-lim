# Task 259: Loudness Panel — Move Pause/Mode Buttons Into Right Panel (Below LUFS Readout)

## Description
In the FabFilter Pro-L 2 reference (`prol2-main-ui.jpg`), the **"||" pause button** and
**"Short Term" mode button** appear **inside the loudness/meter right panel area**, directly
below the large "-13.2 LUFS" numeric readout — not in the bottom status bar.

In M-LIM these two buttons (`pauseMeasurementButton_` and `measurementModeButton_`) are
currently placed in the `ControlStrip` status bar row (far right of the bottom strip). This
creates a layout mismatch: the reference's status bar is clean and minimal, while M-LIM's is
cluttered with these extra controls.

### Reference layout (right-panel bottom, left-to-right):
```
  [  large -13.2 LUFS readout  ]
  [ || ]  [ Short Term ]  [ ↺ ]
  [ Out: 0.0 dBTP             ]
```

### Required change:
1. **Remove** `pauseMeasurementButton_` and `measurementModeButton_` from the `ControlStrip`
   status bar (both `addAndMakeVisible` call and `setBounds` in `resized()`).
2. **Add** them to `LoudnessPanel` — below the large LUFS readout (added by task 209),
   above the existing "Out" readout area.
3. Expose them via public accessors or pass them in at construction time so the panel can
   own or host them.

Note: the `TP` waveform-overlay toggle (`truePeakWaveformButton_`) and loudness-toggle
(`loudnessToggleButton_`) and waveform-mode (`waveformModeButton_`) buttons should also be
evaluated — in the reference they appear near the level meter header, not in the status bar.
This task focuses specifically on the **pause** and **measurement mode** buttons.

## Produces
None

## Consumes
None

## Relevant Files
Modify: `src/ui/ControlStrip.cpp` — remove `pauseMeasurementButton_` and
  `measurementModeButton_` from status-bar setup, `addAndMakeVisible`, and `resized()`.
Modify: `src/ui/ControlStrip.h` — decide whether to keep buttons as members (and pass refs to
  LoudnessPanel) or move ownership entirely. Either approach is acceptable.
Modify: `src/ui/LoudnessPanel.cpp` — add layout region below large LUFS readout for the two
  buttons. Add them via `addAndMakeVisible` and `setBounds` in `resized()`.
Modify: `src/ui/LoudnessPanel.h` — add button members or accept refs/pointers to them.
Modify: `src/PluginEditor.cpp` / `src/PluginEditor.h` — update wiring if buttons move between
  components (ensure APVTS attachments or onClick lambdas remain connected).
Read: `src/ui/Colours.h` — button styling constants.
Read: `/reference-docs/reference-screenshots/prol2-main-ui.jpg` — confirm exact visual layout.

## Acceptance Criteria
- [ ] Build: `cmake --build build -j$(nproc) 2>&1 | tail -3` → Expected: `[100%] Built target MLIM_Standalone`
- [ ] Visual: Launch on Xvfb → status bar row no longer contains `||` or `Short Term` buttons
- [ ] Visual: Loudness/right panel shows `||` and `Short Term` buttons below LUFS readout,
  matching reference layout
- [ ] Functional: `||` pause still pauses loudness metering; `Short Term` still cycles mode
- [ ] RMSE: Status bar RMSE ≤ 16% (improvement from 18.6%)

## Tests
None

## Technical Details
The `measurementModeButton_` currently calls `cycleMeasurementMode()` in `ControlStrip`.
If ownership moves to `LoudnessPanel`, the `onClick` lambda will need to be preserved by
passing a callback. The simplest approach is to keep the `onClick` lambda and APVTS wiring
intact by keeping button ownership in `ControlStrip` but parenting them visually under
`LoudnessPanel` via `addAndMakeVisible` on the loudness panel component — or just move the
member variables and their setup to `LoudnessPanel`.

## Dependencies
Requires task 209 (large LUFS readout must exist to have a "below" region to place buttons)
