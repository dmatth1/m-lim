# Task: Extract Atomic Setter Helper to Reduce LimiterEngine Boilerplate

## Description
`LimiterEngine.cpp` has 7+ setter methods (lines 448-504) that follow an identical pattern:

```cpp
void LimiterEngine::setXxx(float value) {
    if (floatBitsEqual(mXxx.load(std::memory_order_relaxed), value))
        return;
    mXxx.store(value);
    mParamsDirty.store(true);
}
```

This boilerplate is repeated for `setInputGain`, `setOutputCeiling`, `setLookahead`, `setAttack`, `setRelease`, `setChannelLinkTransients`, `setChannelLinkRelease`, `setDitherBitDepth`, `setDitherNoiseShaping`, and `setUnityGain`.

Extract a private helper template to eliminate the repetition and reduce the risk of forgetting `mParamsDirty.store(true)` when adding new parameters.

## Produces
None

## Consumes
None

## Relevant Files
Modify: `M-LIM/src/dsp/LimiterEngine.h` — add private helper method
Modify: `M-LIM/src/dsp/LimiterEngine.cpp` — refactor setter methods to use helper

## Acceptance Criteria
- [ ] Run: `cmake --build build --target MLIM_Standalone_Standalone -j$(nproc) 2>&1 | tail -3` → Expected: build succeeds
- [ ] Run: `cd build && ctest --output-on-failure` → Expected: all tests pass

## Tests
None (existing tests cover all setter behavior)

## Technical Details
Add to LimiterEngine.h (private section):
```cpp
// Helper: store value and mark dirty if changed (float version)
void setIfChanged(std::atomic<float>& param, float newValue) {
    if (!floatBitsEqual(param.load(std::memory_order_relaxed), newValue)) {
        param.store(newValue);
        mParamsDirty.store(true);
    }
}
// Helper: store value and mark dirty if changed (int version)
void setIfChanged(std::atomic<int>& param, int newValue) {
    if (param.load(std::memory_order_relaxed) != newValue) {
        param.store(newValue);
        mParamsDirty.store(true);
    }
}
// Helper: store value and mark dirty if changed (bool version)
void setIfChanged(std::atomic<bool>& param, bool newValue) {
    if (param.load(std::memory_order_relaxed) != newValue) {
        param.store(newValue);
        mParamsDirty.store(true);
    }
}
```

Then each setter becomes a one-liner:
```cpp
void LimiterEngine::setAttack(float ms) { setIfChanged(mAttackMs, ms); }
```

Note: `setInputGain` and `setOutputCeiling` do a dB-to-linear conversion before storing, so they'll be slightly longer but still use the helper for the store+dirty logic.

## Dependencies
None
