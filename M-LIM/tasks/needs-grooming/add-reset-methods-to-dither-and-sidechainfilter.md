# Task: Add Dedicated `reset()` Methods to Dither and SidechainFilter

## Description
`LimiterEngine::reset()` needs to clear DSP state in `Dither` and `SidechainFilter` but these
classes have no `reset()` method. The current workaround calls `prepare()` on them:

```cpp
// In LimiterEngine::reset()
mDitherL.prepare(mSampleRate);              // wrong: full prepare just to zero error state
mDitherR.prepare(mSampleRate);
mSidechainFilter.prepare(mSampleRate, mMaxBlockSize);  // wrong: reallocates JUCE coefficients
```

Calling `prepare()` from `reset()` creates two problems:
1. **Semantic mismatch** — `prepare()` is documented as a setup call for when sample rate/block size
   change. Calling it in `reset()` implies the sample rate has changed when it hasn't.
2. **SidechainFilter fragility** — `SidechainFilter::prepare()` allocates JUCE `IIR::Coefficients`
   objects (via `Coeffs::makeHighPass(...)` etc.). These use ref-counted heap allocation. Calling
   this from `reset()` introduces heap allocation in what should be a lightweight state-clearing path.
   While `reset()` is documented as message-thread-only, the allocation is still unnecessary.

Add proper lightweight `reset()` methods:
- `Dither::reset()` — clears `mError1` and `mError2` only (no coefficient recomputation)
- `SidechainFilter::reset()` — calls `mHP[ch].reset()` etc. on each JUCE IIR filter (clears the
  z-state without touching coefficients)

Then update `LimiterEngine::reset()` to call these methods instead of `prepare()`.

## Produces
None

## Consumes
None

## Relevant Files
Modify: `M-LIM/src/dsp/Dither.h` — add `void reset();` declaration
Modify: `M-LIM/src/dsp/Dither.cpp` — implement `reset()` to zero `mError1` and `mError2`
Modify: `M-LIM/src/dsp/SidechainFilter.h` — add `void reset();` declaration
Modify: `M-LIM/src/dsp/SidechainFilter.cpp` — implement `reset()` to call `mHP[ch].reset()`,
  `mLP[ch].reset()`, `mTiltLS[ch].reset()`, `mTiltHS[ch].reset()` for each channel
Modify: `M-LIM/src/dsp/LimiterEngine.cpp` — in `reset()`: replace `mDitherL.prepare(...)` and
  `mDitherR.prepare(...)` with `mDitherL.reset()` / `mDitherR.reset()`, and replace
  `mSidechainFilter.prepare(...)` with `mSidechainFilter.reset()`

## Acceptance Criteria
- [ ] Run: `grep -n "mDitherL.prepare\|mDitherR.prepare\|mSidechainFilter.prepare" M-LIM/src/dsp/LimiterEngine.cpp` → Expected: no output (calls removed from reset())
- [ ] Run: `cmake --build build --target MLIM_Standalone_Standalone -j$(nproc) 2>&1 | tail -3` → Expected: build succeeds
- [ ] Run: `cd build && ctest --output-on-failure` → Expected: all tests pass

## Tests
None (the reset logic is covered by `tests/dsp/test_dither.cpp` and `tests/dsp/test_sidechain_filter.cpp`
which call prepare then process — the new reset() should be testable by calling it between runs)

## Technical Details
- `juce::dsp::IIR::Filter<float>::reset()` clears the internal delay-line state; it is safe to call
  without touching `coefficients`
- `Dither::reset()` body is 2 lines: `mError1 = 0.0f; mError2 = 0.0f;`
- `SidechainFilter::reset()` should NOT touch `mHPFreq`, `mLPFreq`, `mTiltDb`, or any atomics —
  those are configuration, not state
- Similarly do NOT modify `mCoeffsDirty` in `SidechainFilter::reset()`

## Dependencies
None
