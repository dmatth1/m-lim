# Task 118: displayMode APVTS Parameter Not Wired to WaveformDisplay

## Description
`ParamID::displayMode` is a 5-choice APVTS parameter ("Fast"/"Slow"/"SlowDown"/"Infinite"/"Off")
that is saved/restored with plugin state, but it is completely disconnected from
`WaveformDisplay`:

1. **APVTS → WaveformDisplay** (missing): when `setStateInformation()` loads a project
   or preset, the APVTS `displayMode` value changes but `WaveformDisplay::displayMode_`
   is never updated. The waveform display always ignores the persisted setting.

2. **WaveformDisplay → APVTS** (missing): when the user clicks the on-screen mode
   selector (`WaveformDisplay::mouseDown`), `setDisplayMode()` updates the local
   `displayMode_` member but does NOT write the new value back into APVTS. So the
   user's choice is never actually saved.

Net result: the `displayMode` APVTS parameter is decorative. Automation, preset recall,
and project reload all have no effect on the waveform display mode.

The fix is a two-part wiring inside `PluginEditor`:

**Part A — APVTS → WaveformDisplay** (state restore path):
Add an `APVTS::Listener` (or use `apvts.addParameterListener`) in `MLIMAudioProcessorEditor`
for `ParamID::displayMode`. In the callback, call:
```cpp
waveformDisplay_.setDisplayMode(
    static_cast<WaveformDisplay::DisplayMode>(
        static_cast<int>(newValue)));
```
Also call this once in the constructor (after attachments are created) to sync initial value.

**Part B — WaveformDisplay → APVTS** (user interaction path):
Add a `std::function<void(DisplayMode)> onDisplayModeChanged` callback to `WaveformDisplay`.
Call it from `setDisplayMode()` whenever the mode is changed by user interaction.
In `PluginEditor::wireCallbacks()`, wire it to write the parameter:
```cpp
waveformDisplay_.onDisplayModeChanged = [this](WaveformDisplay::DisplayMode mode)
{
    if (auto* p = apvts.getParameter(ParamID::displayMode))
        p->setValueNotifyingHost(p->convertTo0to1(static_cast<float>(mode)));
};
```

## Produces
None

## Consumes
None

## Relevant Files
Read:    `M-LIM/src/ui/WaveformDisplay.h`  — add `onDisplayModeChanged` callback, update `setDisplayMode` sig
Modify:  `M-LIM/src/ui/WaveformDisplay.cpp` — call `onDisplayModeChanged` from `setDisplayMode()`
Modify:  `M-LIM/src/PluginEditor.h`         — inherit `juce::AudioProcessorValueTreeState::Listener`, add member
Modify:  `M-LIM/src/PluginEditor.cpp`       — register APVTS listener in ctor; wire `onDisplayModeChanged` in `wireCallbacks()`
Read:    `M-LIM/src/Parameters.h`           — `ParamID::displayMode` constant
Read:    `M-LIM/src/PluginProcessor.cpp`    — see TODO comment near `pDisplayMode` in `pushAllParametersToEngine()`

## Acceptance Criteria
- [ ] Run: `cd /workspace/M-LIM && cmake --build build -j$(nproc) 2>&1 | tail -5` → Expected: zero errors.
- [ ] Run: `cd /workspace/M-LIM/build && ctest --output-on-failure 2>&1 | tail -10` → Expected: all tests pass.
- [ ] Manual: click waveform display mode label → cycle through modes → observe that `apvts.getParameter("displayMode")->getValue()` reflects the new mode (verified via a test or log).
- [ ] Manual: set APVTS displayMode to 2 (SlowDown) via `apvts.getRawParameterValue("displayMode")->store(2)` → WaveformDisplay shows "SLOWDOWN" label.

## Tests
- Integration: `tests/integration/test_plugin_processor.cpp` — extend state round-trip test to verify that saving `displayMode = 3` (Infinite) and restoring produces `WaveformDisplay::getDisplayMode() == DisplayMode::Infinite` (requires editor to be created in test).

## Technical Details
- `WaveformDisplay` currently has no APVTS reference and should NOT be given one
  (keeps the component reusable). The `onDisplayModeChanged` callback pattern keeps
  the wiring in `PluginEditor`.
- The `displayMode` APVTS parameter stores values 0–4 matching `WaveformDisplay::DisplayMode`.
- The TODO comment in `PluginProcessor.cpp` line ~263 can be removed once wiring is done.
- `PluginEditor` should add `juce::AudioProcessorValueTreeState::Listener` only for
  `ParamID::displayMode` (or use a `Value`/`ValueListener` on the APVTS state tree).

## Dependencies
None
