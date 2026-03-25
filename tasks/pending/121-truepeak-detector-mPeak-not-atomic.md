# Task 121: TruePeakDetector::mPeak Is Not Atomic — Data Race Between Audio and UI Threads

## Description
`TruePeakDetector::mPeak` (`TruePeakDetector.h` line 68) is declared as a plain
`float` but is written on the **audio thread** in `processSample()` (lines 119,
148) and read on the **UI/message thread** via `getPeak()` (line 163).  This is
a C++ data race: concurrent unsynchronised read+write on a non-atomic object is
undefined behaviour regardless of whether the hardware guarantees atomicity.
The fix is to change the member type to `std::atomic<float>`.

`resetPeak()` also writes `mPeak` (line 172), typically called from the message
thread, compounding the race.

Steps:
1. Change `float mPeak = 0.0f;` to `std::atomic<float> mPeak { 0.0f };` in the header.
2. Update every store to use `mPeak.store(value, std::memory_order_relaxed)`.
3. Update `getPeak()` to return `mPeak.load(std::memory_order_relaxed)`.
4. Update `resetPeak()` to use `mPeak.store(0.0f, std::memory_order_relaxed)`.

`std::memory_order_relaxed` is sufficient here because the peak value is only a
display metric and does not gate any other synchronisation.

## Produces
None

## Consumes
None

## Relevant Files
Modify: `M-LIM/src/dsp/TruePeakDetector.h` — change member type, update getPeak/resetPeak declarations
Modify: `M-LIM/src/dsp/TruePeakDetector.cpp` — update processSample stores, getPeak/resetPeak implementations
Read: `M-LIM/src/dsp/MeterData.h` — understand how the peak value flows to the UI

## Acceptance Criteria
- [ ] Run: `grep -n "mPeak" M-LIM/src/dsp/TruePeakDetector.h` → Expected: line shows `std::atomic<float> mPeak`
- [ ] Run: `grep -n "mPeak" M-LIM/src/dsp/TruePeakDetector.cpp` → Expected: all stores use `.store(`, all loads use `.load(`
- [ ] Run: `cd M-LIM && cmake --build build --target MLIMTests -j$(nproc) && cd build && ctest -R true_peak --output-on-failure` → Expected: all true-peak tests pass

## Tests
- Unit: `tests/dsp/test_true_peak.cpp` — add a test that calls `processSample()` and `getPeak()` and verify the returned value matches; exercise `resetPeak()` and confirm it returns 0.

## Technical Details
- `std::atomic<float>` is lock-free on all modern x86/ARM targets and introduces
  zero overhead in the audio path.
- Do NOT use `std::atomic<float>::exchange` or compare_exchange; simple
  store/load is all that is needed.

## Dependencies
None
