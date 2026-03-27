# Task 467: Add jassert Validation for Parameter Pointers in initParameterPointers()

## Description
`MLIMAudioProcessor::initParameterPointers()` (PluginProcessor.cpp, lines 240-262) retrieves 20 raw parameter pointers via `apvts.getRawParameterValue()` and stores them as `std::atomic<float>*`. The subsequent `pushAllParametersToEngine()` defensively checks each with `if (pParam)`, but if any pointer is null, it means the parameter layout is misconfigured — a bug that should fail loudly during development, not silently skip parameter updates at runtime.

Add `jassert(pXxx != nullptr)` after each `getRawParameterValue()` call to catch parameter ID mismatches during development builds.

## Produces
None

## Consumes
None

## Relevant Files
Modify: `M-LIM/src/PluginProcessor.cpp` — add jassert after each getRawParameterValue call in initParameterPointers()

## Acceptance Criteria
- [ ] Run: `grep -c "jassert" src/PluginProcessor.cpp` → Expected: at least 20 (one per parameter pointer)
- [ ] Run: `cmake --build build --target MLIM_Standalone_Standalone -j$(nproc) 2>&1 | tail -3` → Expected: build succeeds
- [ ] Run: `cd build && ctest --output-on-failure` → Expected: all tests pass

## Tests
None

## Technical Details
After each line like:
```cpp
pInputGain = apvts.getRawParameterValue (ParamID::inputGain);
```
Add:
```cpp
jassert (pInputGain != nullptr);
```

In release builds, `jassert` compiles to nothing, so there's zero runtime cost.

## Dependencies
None
