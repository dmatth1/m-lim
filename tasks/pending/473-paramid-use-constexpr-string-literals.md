# Task 473: Change ParamID Constants from juce::String to constexpr const char*

## Description
`Parameters.h` declares parameter IDs as `inline const juce::String`:

```cpp
inline const juce::String inputGain = "inputGain";
```

Each `juce::String` involves a heap allocation per translation unit that includes the header (even with `inline`, the object is constructed at startup). These IDs are used in two hot paths:

1. `pushAllParametersToEngine()` — called every `processBlock()` — compares against `ParamID::*` strings
2. `parameterChanged()` — compares paramID strings

Using `constexpr const char*` eliminates startup allocations and makes comparisons faster (pointer comparison possible with JUCE parameter IDs).

## Produces
None

## Consumes
None

## Relevant Files
Modify: `M-LIM/src/Parameters.h` — change all `inline const juce::String` to `constexpr const char*`

## Acceptance Criteria
- [ ] Run: `grep "juce::String" src/Parameters.h` → Expected: no juce::String in ParamID namespace (only in function signature)
- [ ] Run: `cmake --build build --target MLIM_Standalone_Standalone -j$(nproc) 2>&1 | tail -3` → Expected: build succeeds
- [ ] Run: `cd build && ctest --output-on-failure` → Expected: all tests pass

## Tests
None

## Technical Details
Change:
```cpp
inline const juce::String inputGain = "inputGain";
```
to:
```cpp
constexpr const char* inputGain = "inputGain";
```

JUCE's `getRawParameterValue()` and `addParameterListener()` accept `juce::StringRef` which implicitly converts from `const char*`, so no call-site changes are needed.

## Dependencies
None
