# Task 504: Persist current preset name in session state

## Description
`MLIMAudioProcessor::getStateInformation()` only serializes the APVTS parameter tree.
`PresetManager::getCurrentPresetName()` is not included. When a DAW session is saved
and reloaded, the APVTS parameters are restored correctly but the preset name is lost.

**Symptom**: After session reload, `PluginEditor` calls:
```cpp
topBar_.setPresetName(audioProcessor.presetManager.getCurrentPresetName());
```
`getCurrentPresetName()` returns `""` (or whatever default it initialises to), so the
TopBar shows no preset name even though all parameters were correctly restored.

**Fix**: Store the current preset name as a child element of the serialized XML (alongside
the APVTS state and ABState introduced by task 490):

```xml
<MLIMState>
  <Parameters>...</Parameters>     <!-- APVTS state -->
  <ABState activeIsA="1">...</ABState>  <!-- task 490 -->
  <PresetName value="My Preset"/>  <!-- this task -->
</MLIMState>
```

On `setStateInformation()`:
1. If `<PresetName>` child is present, call `presetManager.setCurrentPresetName(name)`
2. After the editor is next created, `topBar_.setPresetName(...)` will show the correct name

**Backward compatibility**: If the root element has no `<PresetName>` child (old format),
leave the preset name at its default — no crash, just blank name (same as today).

**Note**: This task is closely related to task 490 (persist ABState). If task 490 has
already been merged and introduced the `<MLIMState>` wrapper, this task should add the
`<PresetName>` element inside that existing wrapper. If task 490 has not been merged,
this task must introduce the `<MLIMState>` wrapper itself and task 490 must be updated
to expect it. Coordinate with task 490 before implementation.

`PresetManager` needs a `setCurrentPresetName(const juce::String&)` setter (or equivalent)
if it does not already have one — check `src/state/PresetManager.h` first.

## Produces
None

## Consumes
None

## Relevant Files
Modify: `src/PluginProcessor.cpp` — `getStateInformation()` and `setStateInformation()`:
  add `<PresetName>` serialization inside the `<MLIMState>` wrapper
Modify: `src/state/PresetManager.h` — add `setCurrentPresetName(const juce::String&)`
  if not already present
Modify: `src/state/PresetManager.cpp` — implement the setter
Read: `src/PluginEditor.cpp` — `topBar_.setPresetName(...)` call in constructor to
  understand what `getCurrentPresetName()` feeds
Read: `tasks/pending/490-persist-ab-state-in-session-state.md` — coordinate with
  ABState persistence to avoid conflicting XML formats
Read: `tests/state/test_preset_manager.cpp` — existing tests for context

## Acceptance Criteria
- [ ] Run: `cmake --build build --target MLIM_Standalone_Standalone -j$(nproc) 2>&1 | tail -3` → Expected: build succeeds
- [ ] Run: `cd build && ctest -R MLIMTests --output-on-failure 2>&1 | tail -10` → Expected: all tests pass
- [ ] Manual: save a preset named "TestPreset", call `getStateInformation()`, load state into a fresh processor via `setStateInformation()`, then verify `presetManager.getCurrentPresetName() == "TestPreset"`

## Tests
- Unit: `tests/integration/test_plugin_processor.cpp::test_preset_name_survives_session_roundtrip` — savePreset("MySetting"), getStateInformation, construct fresh processor, setStateInformation, verify `presetManager.getCurrentPresetName() == "MySetting"`
- Unit: `tests/integration/test_plugin_processor.cpp::test_preset_name_backward_compat_no_tag` — setStateInformation with old-format XML (no `<PresetName>` element), verify no crash and `getCurrentPresetName()` returns `""` or a known default

## Technical Details
`PresetManager::getCurrentPresetName()` is declared in `src/state/PresetManager.h`.
Check whether a corresponding setter exists. If not, a private `juce::String mCurrentPresetName`
field and a public `void setCurrentPresetName(const juce::String&)` setter are sufficient.
The `loadPreset()` / `loadNextPreset()` / `loadPreviousPreset()` methods should also update
`mCurrentPresetName` as they do their work.

## Dependencies
Requires task 490 (persist A/B state) — task 490 introduces the `<MLIMState>` XML wrapper
in `getStateInformation()` / `setStateInformation()`. This task adds `<PresetName>` inside
that wrapper. Must wait for task 490 to merge to avoid conflicting XML format changes.
