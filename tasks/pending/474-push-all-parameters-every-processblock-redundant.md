# Task 474: Avoid Redundant pushAllParametersToEngine() Calls on Every processBlock

## Description
`MLIMAudioProcessor::processBlock()` calls `pushAllParametersToEngine()` on every audio callback (line 129 of PluginProcessor.cpp). This reads ~20 atomic parameter values and calls ~20 setter methods on the LimiterEngine, **even when no parameters have changed**.

The LimiterEngine setters already have change guards (`floatBitsEqual` checks) that skip the store when the value hasn't changed, so the CPU cost is mitigated — but 20 atomic loads + 20 function calls + 20 comparisons per buffer is still wasted work in the common steady-state case.

Two better approaches:
1. **Dirty flag on APVTS side**: Set a single atomic dirty flag in `parameterChanged()` listener; only call `pushAllParametersToEngine()` when dirty.
2. **Selective push**: Only push the parameters that actually changed (using `parameterChanged` callbacks to track which).

Option 1 is simplest and eliminates the overhead in the common case (no parameter automation).

## Produces
None

## Consumes
None

## Relevant Files
Modify: `M-LIM/src/PluginProcessor.h` — add `std::atomic<bool> mParametersDirty {true};`
Modify: `M-LIM/src/PluginProcessor.cpp` — guard `pushAllParametersToEngine()` call with dirty check; set dirty in parameterChanged; add APVTS listener for all params (or use a single catchall)

## Acceptance Criteria
- [ ] Run: `cmake --build build --target MLIM_Standalone_Standalone -j$(nproc) 2>&1 | tail -3` → Expected: build succeeds
- [ ] Run: `cd build && ctest --output-on-failure` → Expected: all tests pass

## Tests
None (existing integration tests cover parameter changes)

## Technical Details
In `processBlock()`:
```cpp
if (mParametersDirty.exchange(false))
    pushAllParametersToEngine();
```

The processor already has `parameterChanged()` for oversampling and lookahead. Registering as a listener for ALL parameters (or adding a generic APVTS::Listener that sets the dirty flag for any change) would allow the dirty-flag optimization.

Note: if host automation is active, the dirty flag will be set every buffer anyway — the optimization helps most in the "no automation, user not touching knobs" case, which is the majority of real-time playback.

## Dependencies
None
