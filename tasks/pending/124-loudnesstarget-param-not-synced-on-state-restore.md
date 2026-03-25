# Task 124: loudnessTarget APVTS Parameter Not Synced to LoudnessPanel on State Restore

## Description
`ParamID::loudnessTarget` is a 5-choice APVTS parameter that controls the reference
LUFS target displayed in `LoudnessPanel`. The wiring is currently one-directional only:

- **LoudnessPanel → APVTS** (present): `LoudnessPanel::onTargetChanged` callback
  writes the new choice index to the APVTS parameter (see `PluginEditor.cpp` ~line 120–127).

- **APVTS → LoudnessPanel** (MISSING): when `setStateInformation()` is called (project
  load, preset recall) while the editor is open, the APVTS `loudnessTarget` value
  changes but `loudnessPanel_.setTargetChoice()` is never called. The panel continues
  to display the old target until the editor is closed and reopened.

The constructor does a one-shot initialisation:
```cpp
auto* param = dynamic_cast<juce::AudioParameterChoice*>(
    audioProcessor.apvts.getParameter(ParamID::loudnessTarget));
if (param != nullptr)
    loudnessPanel_.setTargetChoice(param->getIndex());
```
But this only runs once; it does not observe future APVTS changes.

**Fix**: add an `APVTS::Listener` in `MLIMAudioProcessorEditor` for `ParamID::loudnessTarget`.
In `parameterChanged`, call:
```cpp
loudnessPanel_.setTargetChoice(static_cast<int>(newValue));
```
Remove the one-shot constructor block and replace it with a call to the same listener
after registering it (so initial sync and ongoing sync use the same code path).

## Produces
None

## Consumes
None

## Relevant Files
Read:    `M-LIM/src/PluginEditor.h`    — editor class declaration; add Listener base and deregister in dtor
Modify:  `M-LIM/src/PluginEditor.cpp` — register listener in ctor; implement `parameterChanged` for `loudnessTarget`; wire initial sync
Read:    `M-LIM/src/ui/LoudnessPanel.h` — `setTargetChoice(int)` signature
Read:    `M-LIM/src/Parameters.h`        — `ParamID::loudnessTarget` constant

## Acceptance Criteria
- [ ] Run: `cd /workspace/M-LIM && cmake --build build -j$(nproc) 2>&1 | tail -5` → Expected: zero errors.
- [ ] Run: `cd /workspace/M-LIM/build && ctest --output-on-failure 2>&1 | tail -10` → Expected: all tests pass.
- [ ] Code review: `PluginEditor` inherits `juce::AudioProcessorValueTreeState::Listener`, registers for `ParamID::loudnessTarget` in constructor, and removes the listener in destructor.
- [ ] Code review: `parameterChanged` override calls `loudnessPanel_.setTargetChoice(static_cast<int>(newValue))` for `ParamID::loudnessTarget`.

## Tests
- Integration: `tests/integration/test_plugin_processor.cpp` — create a processor+editor, set `loudnessTarget` to index 3 via `apvts.getParameter(...)->setValueNotifyingHost(...)`, verify `loudnessPanel_.targetChoice_` (or via `getTargetChoice()` if accessor added) equals 3. This requires either exposing a getter or temporarily making the field accessible.

## Technical Details
- If `PluginEditor` already inherits `APVTS::Listener` (e.g., after task 108), reuse that
  infrastructure — just add the `loudnessTarget` case to the existing `parameterChanged`.
- Call `apvts.addParameterListener(ParamID::loudnessTarget, this)` in the constructor
  and `apvts.removeParameterListener(ParamID::loudnessTarget, this)` in the destructor
  (before `setLookAndFeel(nullptr)`).
- The constructor's one-shot sync block should be replaced by calling
  `parameterChanged(ParamID::loudnessTarget, ...)` immediately after listener registration,
  or by a direct call to `loudnessPanel_.setTargetChoice(param->getIndex())` kept as-is
  with the listener added on top.

## Dependencies
None
