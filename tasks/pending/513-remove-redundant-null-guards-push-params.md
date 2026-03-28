# Task 513: Remove Redundant Null Guards in `pushAllParametersToEngine()`

## Description

`PluginProcessor::initParameterPointers()` (line 240) assigns every parameter pointer from APVTS and immediately `jassert`s that each is non-null. This proves that after `initParameterPointers()` the pointers are always valid — which is also enforced structurally because APVTS will never return null for a registered parameter ID.

Yet `pushAllParametersToEngine()` (line 284) guards every single pointer access with `if (pXxx)`, as though the pointers could be null. There are 18 such guards. This contradiction creates cognitive noise:

- Readers wonder: "Can these be null? When?" The jasserts say no; the ifs suggest maybe.
- The ifs suppress crashes silently in release builds where jasserts are disabled — but silent partial-parameter-push is worse than a clean failure.
- Every new parameter added to the codebase must follow this inconsistent dual-pattern.

There is one legitimate exception: line 44 in `PluginProcessor.cpp` uses `pLookahead != nullptr` before `initParameterPointers()` is called (in `getLatencyMs()`). That specific guard is correct and should remain.

**Fix:** Remove all `if (pXxx)` guards from `pushAllParametersToEngine()`. Rely on the jasserts in `initParameterPointers()` and the structural guarantee from APVTS. The function body becomes a flat list of unconditional calls, which clearly communicates intent.

## Produces
None

## Consumes
None

## Relevant Files
Modify: `M-LIM/src/PluginProcessor.cpp` — lines 286-304: remove `if (pXxx)` guard wrapping each `limiterEngine.set...()` call, leaving bare unconditional calls
Read: `M-LIM/src/PluginProcessor.h` — lines 49-90: confirm parameter pointer declarations
Read: `M-LIM/src/PluginProcessor.cpp` — line 44: verify this null check (for `pLookahead` in `getLatencyMs()`) is correctly left in place — it runs before `initParameterPointers()`

## Acceptance Criteria
- [ ] Run: `grep -c "if (p[A-Z]" M-LIM/src/PluginProcessor.cpp` → Expected: 1 (only the pLookahead early-exit guard in getLatencyMs remains)
- [ ] Run: `cd /workspace/M-LIM/build && cmake --build . --target MLIM_Standalone_Standalone -j$(nproc) 2>&1 | tail -5` → Expected: exit 0
- [ ] Run: `cd /workspace/M-LIM/build && ctest --output-on-failure` → Expected: all tests pass

## Tests
None (this is a dead-code/clarity cleanup; existing tests cover the parameter push behaviour)

## Technical Details
The final `pushAllParametersToEngine()` should read:
```cpp
void MLIMAudioProcessor::pushAllParametersToEngine()
{
    limiterEngine.setInputGain(pInputGain->load());
    limiterEngine.setOutputCeiling(pOutputCeiling->load());
    limiterEngine.setAlgorithm(static_cast<LimiterAlgorithm>(static_cast<int>(pAlgorithm->load())));
    // ... (all 18 lines, no if-guards)
}
```
The `jassert`s in `initParameterPointers()` already abort in debug builds if a pointer is null. Removing the ifs makes the release-build behaviour consistent: any null dereference surfaces immediately as a crash rather than a silent partial push.

## Dependencies
None
