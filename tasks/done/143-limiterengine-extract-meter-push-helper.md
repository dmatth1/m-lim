# Task 143: Extract metering snapshot into helper in LimiterEngine::process()

## Description
`LimiterEngine::process()` is 176 lines and handles 11 distinct steps including DSP chain
execution AND metering AND FIFO push all in one function. The final section (lines 300-334)
—measuring output, running true peak detection, computing total GR, and building/pushing
`MeterData`—is a distinct concern that can be cleanly extracted.

Extract lines 300-334 into a private method
`snapAndPushMeterData(const juce::AudioBuffer<float>&, float inLevelL, float inLevelR,
float totalGR, int numChannels, int numSamples)` to separate the metering concern from the
DSP chain processing concern.

## Produces
None

## Consumes
None

## Relevant Files
Modify: `M-LIM/src/dsp/LimiterEngine.cpp` — extract lines 300-334 into private helper
Modify: `M-LIM/src/dsp/LimiterEngine.h` — declare private `snapAndPushMeterData()` method

## Acceptance Criteria
- [ ] Run: `awk '/^void LimiterEngine::process/,/^}/' M-LIM/src/dsp/LimiterEngine.cpp | wc -l` → Expected: output is less than 150 (was 176)
- [ ] Run: `grep -n "snapAndPushMeterData" M-LIM/src/dsp/LimiterEngine.cpp` → Expected: at least 2 hits (definition + call site)
- [ ] Run: `cmake --build build -j$(nproc) 2>&1 | grep -E "error:"` → Expected: no output

## Tests
None

## Technical Details
New private method signature in `LimiterEngine.h`:
```cpp
void snapAndPushMeterData(const juce::AudioBuffer<float>& buffer,
                          float inLevelL, float inLevelR,
                          float totalGR, int numChannels, int numSamples);
```

The extracted body:
- Measures output levels via `peakLevel()`
- Runs `mTruePeakL/R.processBlock()` if enabled
- Builds `MeterData md` struct
- Calls `mMeterFIFO.push(md)`

The `totalGR` already computed in `process()` is passed in; `inLevelL/R` were computed at the
start of `process()`. No state changes needed — this is pure data collection and output.

## Dependencies
None
