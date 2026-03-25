# Task 189: Extract LimiterEngine::process() Steps into Private Methods

## Description
`LimiterEngine::process()` (`LimiterEngine.cpp` lines 159–314) is a 155-line function that executes 11 sequential DSP steps inline, each already annotated with a comment block:

1. Apply input gain
2. Copy sidechain + run sidechain filter
3. Upsample
4. TransientLimiter (Stage 1)
5. LevelingLimiter (Stage 2)
6. Downsample
7. Apply output ceiling
8. DC filter
9. Dither
10. Delta mode
11. Meter + push FIFO

The bypass path is an additional 20-line early return at lines 180–199 with duplicated metering logic.

The function is hard to unit-test, hard to profile, and hard to modify safely. Extract each numbered step into a named private method. The calling code in `process()` becomes a readable sequence of `stepApplyInputGain()`, `stepRunSidechainFilter()`, etc.

## Produces
None

## Consumes
None

## Relevant Files
Modify: `M-LIM/src/dsp/LimiterEngine.cpp` — extract step methods from `process()`
Modify: `M-LIM/src/dsp/LimiterEngine.h` — declare the new private methods
Read: `M-LIM/src/dsp/LimiterEngine.h` — understand existing member variables and method layout
Skip: `M-LIM/tests/` — existing integration tests cover the full chain; no new tests required

## Acceptance Criteria
- [ ] Run: `awk '/^void LimiterEngine::process/{found=1} found{count++} /^}$/{if(found){print count; found=0; count=0}}' M-LIM/src/dsp/LimiterEngine.cpp` → Expected: prints a number ≤ 30 (process() body collapsed to orchestration calls only)
- [ ] Run: `cmake --build build -j$(nproc) 2>&1 | tail -5` → Expected: build succeeds, exit 0
- [ ] Run: `cd build && ctest --output-on-failure -R "test_limiter_engine|test_realtime_safety|test_dsp_components" 2>&1 | tail -15` → Expected: all tests pass

## Tests
None

## Technical Details
Suggested private method signatures (add to `LimiterEngine.h` private section):
```cpp
void stepApplyInputGain (juce::AudioBuffer<float>& buffer, int numChannels, int numSamples);
void stepRunSidechainFilter (juce::AudioBuffer<float>& buffer, int numChannels, int numSamples);
void stepUpsample (juce::AudioBuffer<float>& buffer, int numChannels,
                   juce::dsp::AudioBlock<float>& upBlock,
                   juce::dsp::AudioBlock<float>& upSideBlock);
void stepApplyCeiling (juce::AudioBuffer<float>& buffer, int numChannels,
                       int numSamples, float inputGain);
void stepDCFilter (juce::AudioBuffer<float>& buffer, int numChannels, int numSamples);
void stepDither (juce::AudioBuffer<float>& buffer, int numChannels, int numSamples);
void stepDeltaMode (juce::AudioBuffer<float>& buffer, int numChannels, int numSamples);
void pushBypassMeterData (juce::AudioBuffer<float>& buffer,
                          float inLevelL, float inLevelR,
                          int numChannels, int numSamples);
```

The bypass path's duplicated meter push can be extracted as `pushBypassMeterData()`.

Keep `applyPendingParams()` and `snapAndPushMeterData()` (already extracted). The final `process()` body should read like a numbered recipe, not inline DSP code.

**Constraint:** All existing member variables (`mUpPtrs`, `mSidePtrs`, `mSidechainBuffer`, etc.) remain members; no new heap allocations. Audio-thread safety must not regress.

## Dependencies
Requires task 182 (both modify LimiterEngine.cpp — setter guards land first)
