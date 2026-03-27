# Task 462: Fix LimiterEngine::reset() Real-Time Safety — Heap Allocations on Audio Thread

## Description
`LimiterEngine::reset()` (src/dsp/LimiterEngine.cpp, lines 100-144) is called from the audio thread via `PluginProcessor::reset()` (src/PluginProcessor.cpp, line 91). It contains three calls that perform **heap allocations**, violating real-time safety:

1. **Line 130**: `mSidechainFilter.prepare(mSampleRate, mMaxBlockSize)` — calls `Coeffs::makeHighPass()`, `makeLowPass()`, `makeLowShelf()`, `makeHighShelf()` which each allocate a new `ReferenceCountedObject` on the heap (SidechainFilter.cpp lines 29-32).

2. **Lines 133-134**: `mOversampler.prepare(...)` and `mSidechainOversampler.prepare(...)` — each calls `recreate()` which does `std::make_unique<juce::dsp::Oversampling<float>>(...)` and `initProcessing()` (Oversampler.cpp lines 52-69), both heavy heap allocations.

These were introduced by task 430 which added `LimiterEngine::reset()`. The task's description says "clear all DSP state without reallocating" but the implementation calls `prepare()` methods that do allocate.

## Produces
None

## Consumes
None

## Relevant Files
Modify: `src/dsp/LimiterEngine.cpp` — replace allocating `prepare()` calls in `reset()` with non-allocating alternatives
Modify: `src/dsp/SidechainFilter.h` — add a `reset()` method that clears filter state without reallocating coefficients
Modify: `src/dsp/SidechainFilter.cpp` — implement `reset()` using `mHP[ch].reset()`, `mLP[ch].reset()`, etc. (JUCE IIR::Filter::reset() just zeros the state array, no allocation)
Modify: `src/dsp/Oversampler.h` — add a `reset()` method
Modify: `src/dsp/Oversampler.cpp` — implement `reset()` that calls `mOversampling->reset()` (JUCE's Oversampling::reset() clears internal filter state without reallocation)
Modify: `src/dsp/LevelingLimiter.h` — add a `reset()` method
Modify: `src/dsp/LevelingLimiter.cpp` — implement `reset()` that fills mGainState and mEnvState with 1.0f and resets mCurrentGRdB without calling assign() (which could allocate if capacity changes)
Read: `src/PluginProcessor.cpp` — see line 91 where `limiterEngine.reset()` is called from audio thread

## Acceptance Criteria
- [ ] Run: `grep -n "\.prepare(" src/dsp/LimiterEngine.cpp | grep -v "^[[:space:]]*//"` → Expected: no `prepare()` calls inside the `reset()` method (lines 100-144); prepare calls should only appear in the `prepare()` method
- [ ] Run: `grep -n "void reset" src/dsp/SidechainFilter.h` → Expected: `void reset();` declaration present
- [ ] Run: `grep -n "void reset" src/dsp/Oversampler.h` → Expected: `void reset();` declaration present
- [ ] Run: `cmake --build build --target MLIM_Standalone_Standalone -j$(nproc) 2>&1 | tail -3` → Expected: build succeeds
- [ ] Run: `cd build && ctest --output-on-failure` → Expected: all tests pass

## Tests
- Unit: `tests/dsp/test_limiter_engine.cpp::test_reset_no_allocation` — wrap `LimiterEngine::reset()` with an allocation guard (override malloc/new), assert zero allocations occur during the call

## Technical Details
**Fix approach:**

1. Add `SidechainFilter::reset()`:
```cpp
void SidechainFilter::reset()
{
    for (int ch = 0; ch < kMaxChannels; ++ch)
    {
        mHP[ch].reset();
        mLP[ch].reset();
        mTiltLS[ch].reset();
        mTiltHS[ch].reset();
    }
}
```
JUCE's `IIR::Filter::reset()` just calls `std::fill` on the state array — zero allocations.

2. Add `Oversampler::reset()`:
```cpp
void Oversampler::reset()
{
    if (mOversampling)
        mOversampling->reset();
}
```
JUCE's `dsp::Oversampling::reset()` clears internal filter state without reallocating buffers.

3. In `LimiterEngine::reset()`, replace:
   - Line 130: `mSidechainFilter.prepare(...)` → `mSidechainFilter.reset()`
   - Line 133: `mOversampler.prepare(...)` → `mOversampler.reset()`
   - Line 134: `mSidechainOversampler.prepare(...)` → `mSidechainOversampler.reset()`
   - Line 112: `mLevelingLimiter.prepare(...)` → `mLevelingLimiter.reset()` — prepare() calls `std::vector::assign()` which can allocate; a dedicated reset() should use `std::fill()` on existing vectors instead

4. Add `LevelingLimiter::reset()`:
```cpp
void LevelingLimiter::reset()
{
    std::fill(mGainState.begin(), mGainState.end(), 1.0f);
    std::fill(mEnvState.begin(), mEnvState.end(), 1.0f);
    mCurrentGRdB = 0.0f;
}
```

## Dependencies
None
