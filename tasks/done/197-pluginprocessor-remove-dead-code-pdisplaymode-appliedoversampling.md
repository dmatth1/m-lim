# Task 216: Remove dead-code members pDisplayMode and mAppliedOversamplingFactor

## Description
Two private members of `MLIMAudioProcessor` are dead code:

1. **`pDisplayMode`** (`std::atomic<float>*`, `PluginProcessor.h` line 90) — initialised in `initParameterPointers()` but never read anywhere in the audio or message thread. `displayMode` is a UI-only parameter; the engine has no setter for it and it is not pushed in `pushAllParametersToEngine()`. The APVTS parameter value is read directly by the editor and via the `parameterChanged` listener on the editor.

2. **`mAppliedOversamplingFactor`** (`int`, `PluginProcessor.h` line 94) — written in both `prepareToPlay()` and `handleAsyncUpdate()` but never read. It was presumably a guard to prevent double-application of an oversampling change; that logic now lives entirely inside the atomic `mOversamplingChangePending` flag and the engine's own `mCurrentOversamplingFactor`.

Remove both members and all references to them to eliminate confusion about the intended data flow.

## Produces
None

## Consumes
None

## Relevant Files
Modify: `src/PluginProcessor.h` — remove the two member declarations
Modify: `src/PluginProcessor.cpp` — remove the three assignment sites:
  - `pDisplayMode = apvts.getRawParameterValue(ParamID::displayMode);` in `initParameterPointers()`
  - `mAppliedOversamplingFactor = factor;` in `prepareToPlay()`
  - `mAppliedOversamplingFactor = factor;` in `handleAsyncUpdate()`

## Acceptance Criteria
- [ ] Run: `grep -n "pDisplayMode\|mAppliedOversamplingFactor" src/PluginProcessor.h src/PluginProcessor.cpp` → Expected: no matches
- [ ] Run: `cmake --build build -j4 2>&1 | tail -5` → Expected: build succeeds with 0 errors
- [ ] Run: `cd build && ctest --output-on-failure 2>&1 | tail -10` → Expected: all tests pass

## Tests
None (dead code removal — no behavioural change)

## Technical Details
No functional change; just deletion of unused declarations and their assignments.

After removing `pDisplayMode`, confirm that `ParamID::displayMode` is still used correctly:
- The editor registers `addParameterListener(ParamID::displayMode, this)` and reads it directly via `apvts.getParameter()` — no change needed there.

## Dependencies
Requires task 210 (both modify PluginProcessor.cpp — drain FIFO task first to avoid conflicts)
