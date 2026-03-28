# Task: Use JUCE FloatVectorOperations for peak level scanning

## Description
`LimiterEngine::peakLevel()` (lines 204–212) uses a scalar loop to find the maximum absolute value in a buffer:
```cpp
float LimiterEngine::peakLevel(const juce::AudioBuffer<float>& buf, int channel, int numSamples) noexcept
{
    const float* ptr = buf.getReadPointer(channel);
    float level = 0.0f;
    for (int s = 0; s < numSamples; ++s)
        level = std::max(level, std::abs(ptr[s]));
    return level;
}
```
This function is called 4 times per `process()` block (input L, input R, output L, output R) and 2 more times during bypass. JUCE provides `juce::FloatVectorOperations::findAbsoluteMaximum()` which uses SSE/NEON SIMD intrinsics internally and is significantly faster for buffer sizes ≥ 64 samples.

This is a hot-path SIMD optimization opportunity — the function processes every sample in the block and is called multiple times per block.

## Produces
None

## Consumes
None

## Relevant Files
Modify: `M-LIM/src/dsp/LimiterEngine.cpp` — `peakLevel()` static method (lines 204–212)

## Acceptance Criteria
- [ ] Run: `cd build && ctest --output-on-failure` → Expected: all tests pass, exit 0
- [ ] Run: `grep -n "findAbsoluteMaximum\|FloatVectorOperations" M-LIM/src/dsp/LimiterEngine.cpp` → Expected: at least one match showing the SIMD function is used

## Tests
None (performance optimization only, no behavior change)

## Technical Details
Replace the scalar loop with:
```cpp
float LimiterEngine::peakLevel(const juce::AudioBuffer<float>& buf, int channel, int numSamples) noexcept
{
    return juce::FloatVectorOperations::findAbsoluteMaximum(buf.getReadPointer(channel), numSamples);
}
```
This is a one-line change. `findAbsoluteMaximum` returns 0.0f for empty buffers, matching the current behavior. The JUCE function handles alignment and uses SSE2/NEON internally.

## Dependencies
None
