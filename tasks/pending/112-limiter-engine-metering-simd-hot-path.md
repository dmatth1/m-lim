# Task 112: LimiterEngine — Input/Output Metering Loops Use getSample() Preventing SIMD Auto-Vectorization

## Description

`LimiterEngine::process()` contains two inner-loop peak-detection passes on the audio buffer — one for input metering (lines 163–170) and one for output metering (lines 301–307):

```cpp
// Input metering (lines 163–170):
for (int i = 0; i < numSamples; ++i)
{
    inLevelL = std::max(inLevelL, std::abs(buffer.getSample(0, i)));
    if (numChannels > 1)
        inLevelR = std::max(inLevelR, std::abs(buffer.getSample(1, i)));
    else
        inLevelR = inLevelL;
}

// Output metering (lines 301–307):
for (int i = 0; i < numSamples; ++i)
{
    outLevelL = std::max(outLevelL, std::abs(buffer.getSample(0, i)));
    if (numChannels > 1)
        outLevelR = std::max(outLevelR, std::abs(buffer.getSample(1, i)));
}
```

`juce::AudioBuffer::getSample(ch, i)` calls `getReadPointer(ch)[i]` with bounds checking overhead. This pattern:
1. Issues two virtual-ish function calls per sample (getSample on ch=0 and ch=1)
2. Re-evaluates the `numChannels > 1` branch every sample — branch in the inner loop
3. Prevents the compiler from auto-vectorizing the abs-max reduction because the access pattern is not recognized as a simple strided array walk

The same work done with direct channel pointers is 3–4× faster and enables SIMD vectorization. At a 512-sample block with 2 channels, this is 2048 unnecessary function calls per process() call.

**Fix:** Hoist channel pointers outside the loop and process L and R channels in separate passes:

```cpp
// Input metering:
const float* inPtrL = buffer.getReadPointer(0);
for (int i = 0; i < numSamples; ++i)
    inLevelL = std::max(inLevelL, std::abs(inPtrL[i]));

if (numChannels > 1)
{
    const float* inPtrR = buffer.getReadPointer(1);
    for (int i = 0; i < numSamples; ++i)
        inLevelR = std::max(inLevelR, std::abs(inPtrR[i]));
}
else
{
    inLevelR = inLevelL;
}
```

Apply the same refactoring to the output metering block (lines 301–307).

## Produces
None

## Consumes
None

## Relevant Files
Modify: `M-LIM/src/dsp/LimiterEngine.cpp` — refactor input metering (lines 163–170) and output metering (lines 301–307)
Read: `M-LIM/tests/dsp/test_limiter_engine.cpp` — existing tests to ensure they still pass

## Acceptance Criteria
- [ ] Run: `cd /workspace/M-LIM/build && ctest -R LimiterEngine --output-on-failure` → Expected: all tests pass, exit 0
- [ ] Run: `grep -n "getSample" /workspace/M-LIM/src/dsp/LimiterEngine.cpp` → Expected: no occurrences of getSample in the input/output metering loops (getSample removed from the hot path)
- [ ] Run: `cd /workspace/M-LIM/build && ctest --output-on-failure` → Expected: all tests pass, exit 0

## Tests
- Unit: `tests/dsp/test_limiter_engine.cpp::test_metering_levels_correct_after_refactor` — process a known sine wave at 0.5 amplitude through the engine in bypass mode; assert that `MeterData.inputLevelL` and `inputLevelR` equal 0.5 ± 0.001 (confirming the refactored metering produces the same values as before)
- Unit: `tests/dsp/test_limiter_engine.cpp::test_mono_metering_mirrors_L_to_R` — prepare engine with 1 channel; process a signal; assert `inputLevelR == inputLevelL` and `outputLevelR == outputLevelL`

## Technical Details

The refactoring is purely mechanical — no change to the computed values. The two metering blocks become:

**Input metering (replace lines 162–170):**
```cpp
const float* inPtrL = buffer.getReadPointer(0);
for (int i = 0; i < numSamples; ++i)
    inLevelL = std::max(inLevelL, std::abs(inPtrL[i]));

if (numChannels > 1)
{
    const float* inPtrR = buffer.getReadPointer(1);
    for (int i = 0; i < numSamples; ++i)
        inLevelR = std::max(inLevelR, std::abs(inPtrR[i]));
}
else
{
    inLevelR = inLevelL;
}
```

**Output metering (replace lines 300–308):**
```cpp
const float* outPtrL = buffer.getReadPointer(0);
for (int i = 0; i < numSamples; ++i)
    outLevelL = std::max(outLevelL, std::abs(outPtrL[i]));

if (numChannels > 1)
{
    const float* outPtrR = buffer.getReadPointer(1);
    for (int i = 0; i < numSamples; ++i)
        outLevelR = std::max(outLevelR, std::abs(outPtrR[i]));
}
else
{
    outLevelR = outLevelL;
}
```

## Dependencies
None
