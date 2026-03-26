# Task 285: TruePeakDetector FIR state erased per-block in enforcement path causes boundary blind spot

## Description

`LimiterEngine::stepEnforceTruePeak()` calls `TruePeakDetector::reset()` before every audio block:

```cpp
// LimiterEngine.cpp — stepEnforceTruePeak()
mTruePeakEnforceL.reset();   // ← zeros BOTH mPeak AND the 12-tap FIR history
mTruePeakEnforceL.processBlock(buffer.getReadPointer(0), numSamples);
float tpL = mTruePeakEnforceL.getPeak();
```

`TruePeakDetector::reset()` (TruePeakDetector.cpp:190–196) zeros `mLinearBuf` and `mBuffer` — the FIR filter history arrays. After reset, the detector needs ~6 samples of warm-up before its FIR window contains valid audio data. During warm-up, inter-sample peaks are computed with zeroed look-behind samples, systematically underestimating the true peak.

**Effect**: For the first 6 samples of every block, inter-sample peaks above the output ceiling are not detected and pass through un-corrected. For typical block sizes (512 samples) this is only 1.2% of samples, but it is **systematic** — it happens at the boundary of every block, and peaks at block boundaries are not rare (the limiter's hard-clip may produce abrupt edges precisely there).

The reset is needed to measure only the current block's peak (not carry over a peak from a previous block indefinitely). But `reset()` should only clear `mPeak`, not the FIR state.

## Produces
None

## Consumes
None

## Relevant Files
Modify: `M-LIM/src/dsp/TruePeakDetector.h` — add `resetPeak()` declaration
Modify: `M-LIM/src/dsp/TruePeakDetector.cpp` — implement `resetPeak()`
Modify: `M-LIM/src/dsp/LimiterEngine.cpp` — replace `reset()` with `resetPeak()` in `stepEnforceTruePeak()`

## Acceptance Criteria
- [ ] Run: `cd M-LIM/build && ctest -R TruePeak --output-on-failure` → Expected: all TruePeakDetector tests pass
- [ ] Run: `grep -n "\.reset()" M-LIM/src/dsp/LimiterEngine.cpp` → Expected: no `mTruePeakEnforce` reset calls; enforcement path uses `resetPeak()` instead
- [ ] Run: `grep -n "resetPeak" M-LIM/src/dsp/TruePeakDetector.h` → Expected: one match (declaration)

## Tests
- Unit: Create a test that calls `processSample()` for 12 samples of a 0.9 sine wave (to warm up the FIR), then calls `resetPeak()` (not `reset()`), then calls `processSample()` with one more sample, and asserts the FIR output is close to what the warmed-up filter would produce (not the cold-start ~0 output).
- Unit: Verify that after `resetPeak()`, `getPeak()` returns 0.0 (peak accumulator was cleared) but the filter output for the next sample is consistent with a warm FIR (state preserved).

## Technical Details

**Fix 1** — Add `resetPeak()` to `TruePeakDetector`:

In `TruePeakDetector.h`:
```cpp
/** Reset only the running peak accumulator, preserving FIR filter state.
 *  Use this when starting a new measurement window (e.g., per-block enforcement)
 *  to avoid a cold-FIR warm-up gap at block boundaries.
 *  Use reset() only when completely reinitializing (e.g., after prepare()). */
void resetPeak();
```

In `TruePeakDetector.cpp`:
```cpp
void TruePeakDetector::resetPeak()
{
    mPeak.store(0.0f, std::memory_order_relaxed);
    // FIR state (mLinearBuf, mBuffer, mLinearPos, mWritePos) intentionally preserved.
}
```

**Fix 2** — Use `resetPeak()` in enforcement path in `LimiterEngine.cpp::stepEnforceTruePeak()`:

```cpp
void LimiterEngine::stepEnforceTruePeak(...)
{
    if (!mTruePeakEnabled.load()) return;

    mTruePeakEnforceL.resetPeak();   // ← was reset(); FIR state now preserved
    mTruePeakEnforceL.processBlock(buffer.getReadPointer(0), numSamples);
    float tpL = mTruePeakEnforceL.getPeak();

    float tpR = tpL;
    if (numChannels > 1)
    {
        mTruePeakEnforceR.resetPeak();   // ← was reset()
        mTruePeakEnforceR.processBlock(buffer.getReadPointer(1), numSamples);
        tpR = mTruePeakEnforceR.getPeak();
    }
    ...
}
```

The enforcement TruePeakDetectors (`mTruePeakEnforceL/R`) are separate from the metering detectors (`mTruePeakL/R`), which correctly never reset their state between blocks. By preserving FIR state in the enforcement detectors, we guarantee BS.1770-4 compliant inter-sample peak detection across the full block including the first sample.

**Note**: Full `reset()` should still be called in `prepare()` (via the existing call chain) to initialise state when the sample rate changes. Do not change those call sites.

## Dependencies
None
