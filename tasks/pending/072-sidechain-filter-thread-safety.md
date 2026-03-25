# Task 072: Thread-Safe Parameter Updates for SidechainFilter

## Description
SidechainFilter::setHighPassFreq(), setLowPassFreq(), and setTilt() each call updateCoefficients() which recalculates IIR filter coefficients using JUCE's `IIR::Coefficients::makeHighPass()` etc. These methods:

1. Allocate new `ReferenceCountedObject` for coefficients (heap allocation)
2. Assign new coefficients to filters via `filter.coefficients = ...`

If these setters are called from the UI/message thread while process() is running on the audio thread, there's a data race on the filter's coefficient pointer. JUCE's `IIR::Filter` does NOT use atomic coefficient swapping by default.

Additionally, in the planned PluginProcessor integration (task 017), if these setters are called from processBlock after reading APVTS values, the heap allocation from `makeHighPass()` etc. violates real-time safety.

## Produces
None

## Consumes
None

## Relevant Files
Modify: `M-LIM/src/dsp/SidechainFilter.h` — add atomic/deferred parameter change mechanism
Modify: `M-LIM/src/dsp/SidechainFilter.cpp` — defer coefficient recalculation to process() or use lock-free coefficient swap
Read: `M-LIM/src/dsp/SidechainFilter.h` — current interface

## Acceptance Criteria
- [ ] Run: `grep -n "updateCoefficients\|makeHighPass\|makeLowPass" M-LIM/src/dsp/SidechainFilter.cpp` → Expected: coefficient construction NOT called from setters; deferred to safe context
- [ ] Run: `cd M-LIM && cmake --build build --target MLIMTests -j$(nproc) && cd build && ctest -R sidechain --output-on-failure` → Expected: all tests pass

## Tests
- Unit: `tests/dsp/test_sidechain_filter.cpp::test_parameter_change_during_process` — call setHighPassFreq() while process() is active on another thread, verify no crash

## Technical Details
**Recommended approach (matches LimiterEngine pattern):**
1. Store desired values in atomics: `std::atomic<float> mPendingHP, mPendingLP, mPendingTilt;`
2. Add `std::atomic<bool> mCoeffsDirty{false};`
3. Setters only update atomics and set dirty flag
4. At the start of process(), check dirty flag; if set, recalculate coefficients
5. The allocation from makeHighPass/makeLowPass is still on audio thread — to fully fix this, use pre-computed biquad coefficients (manual formula) instead of JUCE's allocating coefficient factory methods

**Alternative (simpler but still allocates):**
Use juce::dsp::ProcessorDuplicator with SmoothedValue for coefficient interpolation. This is more complex but provides glitch-free coefficient changes.

## Dependencies
Requires task 009
