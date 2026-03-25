# Task 077: displayMode APVTS Parameter Is Registered But Never Applied

## Description
`Parameters.h` declares `ParamID::displayMode` and `createParameterLayout()`
registers a 5-choice parameter ("Fast"/"Slow"/"SlowDown"/"Infinite"/"Off")
in the APVTS. However:

1. `PluginProcessor.h` has no `pDisplayMode` raw-parameter pointer.
2. `PluginProcessor::initParameterPointers()` never calls
   `apvts.getRawParameterValue(ParamID::displayMode)`.
3. `pushAllParametersToEngine()` and `processBlock()` never read or push it.
4. No DSP component has a `setDisplayMode()` method.
5. No UI component reads this parameter from the APVTS — it is completely inert.

This means the display-mode parameter is saved and restored in plugin state
(because it's in the APVTS) but has absolutely no effect. When the WaveformDisplay
is implemented (task 022/045), this parameter needs to be wired to it. Until then,
the dangling declaration misleads developers into thinking it is already hooked up.

## Produces
None

## Consumes
None

## Relevant Files
Read: `M-LIM/src/Parameters.h` — `ParamID::displayMode` declared (line 29)
Read: `M-LIM/src/Parameters.cpp` — parameter registered (lines 172–179)
Read: `M-LIM/src/PluginProcessor.h` — no `pDisplayMode` member
Read: `M-LIM/src/PluginProcessor.cpp` — `initParameterPointers` has no displayMode entry

Modify: `M-LIM/src/PluginProcessor.h` — add `std::atomic<float>* pDisplayMode = nullptr;`
Modify: `M-LIM/src/PluginProcessor.cpp` — fetch the parameter in
  `initParameterPointers()`; add a TODO comment in `pushAllParametersToEngine()`
  indicating that the UI (WaveformDisplay) should read this parameter directly
  from the APVTS rather than via the DSP engine.

## Acceptance Criteria
- [ ] Run: `grep -n "displayMode" M-LIM/src/PluginProcessor.h M-LIM/src/PluginProcessor.cpp` → Expected: at least 2 matches showing pDisplayMode declaration and getRawParameterValue call.
- [ ] Run: `cd M-LIM && cmake --build build -j$(nproc) 2>&1 | tail -3` → Expected: builds without errors.

## Tests
None — parameter registration only; no DSP behavior changes.

## Technical Details
The displayMode parameter controls the WaveformDisplay's scrolling speed, not
the DSP chain. It should be read directly by the WaveformDisplay UI component via
`apvts.getRawParameterValue(ParamID::displayMode)` when that component is
implemented. The processor need not push it to the engine — just fetch the pointer
so it's not a dangling APVTS entry.

## Dependencies
None
