# Task: Replace Duplicated L/R DSP Components with Per-Channel Arrays in LimiterEngine

## Description
`LimiterEngine` declares separate L/R instances of several DSP components:

```cpp
TruePeakDetector mTruePeakL, mTruePeakR;           // lines 123-124
TruePeakDetector mTruePeakEnforceL, mTruePeakEnforceR; // lines 125-126
DCFilter mDCFilterL, mDCFilterR;                    // lines 128-129
Dither mDitherL, mDitherR;                          // lines 130-131
```

Every operation on these is duplicated with near-identical code — `prepare()`, `reset()`, `process()`, `setBitDepth()`, etc. are all called twice with L then R variants. This pattern:

1. Makes every change require editing two parallel call sites
2. Cannot scale to surround formats (5.1/7.1) without 6/8 duplicated lines per operation
3. Is inconsistent with `TransientLimiter` and `LevelingLimiter` which already handle multi-channel internally

Replace with `std::array<TruePeakDetector, kMaxChannels>` (etc.) and loop over channels.

## Produces
None

## Consumes
None

## Relevant Files
Modify: `M-LIM/src/dsp/LimiterEngine.h` — replace paired L/R members with arrays (lines 123–131)
Modify: `M-LIM/src/dsp/LimiterEngine.cpp` — replace all paired L/R calls with channel loops in `prepare()`, `reset()`, `process()`, `stepDCFilter()`, `stepDither()`, `stepEnforceTruePeak()`, `snapAndPushMeterData()`

## Acceptance Criteria
- [ ] Run: `grep -c "mTruePeakL\|mTruePeakR\|mDCFilterL\|mDCFilterR\|mDitherL\|mDitherR" src/dsp/LimiterEngine.h` → Expected: 0
- [ ] Run: `cmake --build build --target MLIM_Standalone_Standalone -j$(nproc) 2>&1 | tail -3` → Expected: build succeeds
- [ ] Run: `cd build && ctest --output-on-failure` → Expected: all tests pass

## Tests
None (existing tests cover the DSP behavior)

## Technical Details
Use `static constexpr int kMaxChannels = 2;` in LimiterEngine (matching TransientLimiter's pattern). Replace:
```cpp
DCFilter mDCFilterL, mDCFilterR;
```
with:
```cpp
std::array<DCFilter, kMaxChannels> mDCFilters;
```
And replace paired calls like:
```cpp
mDCFilterL.process(buffer.getWritePointer(0), numSamples);
if (numChannels > 1)
    mDCFilterR.process(buffer.getWritePointer(1), numSamples);
```
with:
```cpp
for (int ch = 0; ch < numChannels; ++ch)
    mDCFilters[ch].process(buffer.getWritePointer(ch), numSamples);
```

## Dependencies
None
