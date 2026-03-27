# Task: Add SIMD Stereo Fast Path to LevelingLimiter

## Description

`LevelingLimiter::process()` has no SIMD optimization for the stereo per-sample gain
smoothing loop, while `TransientLimiter::process()` has one. At 32x oversampling
(1,411,200 Hz effective rate), this loop executes ~1.4 million iterations per second
per audio buffer — making it a hot path.

**File**: `M-LIM/src/dsp/LevelingLimiter.cpp`, lines 180–235

**Opportunity**: The inner loop (steps 3 and 4 per sample) processes two independent
channels (for stereo). The attack/release IIR and gain application for both channels
can be computed in parallel using JUCE `dsp::SIMDRegister<float>` (4-wide float),
matching the pattern already used in `TransientLimiter.cpp` lines 352–400.

The approach is:
1. Load `mGainState[0]` and `mGainState[1]` into SIMD lanes 0–1 (lanes 2–3 = 1.0f)
2. Load `perChRequiredGain[0]` and `perChRequiredGain[1]` into target lanes
3. Compute attack and release gains simultaneously for both channels
4. Select attack/release per-lane using `SIMDf::lessThan` mask (branchless)
5. Clamp to `[kMinGain, 1.0f]`
6. Apply gain to `channelData[ch][s]` for both channels

Use `#if JUCE_USE_SIMD` guard and keep the existing scalar path as fallback for
mono/surround, matching the pattern in `TransientLimiter.cpp`.

Note: the `mEnvState` adaptive smoother update (step 3a, line 205–210) must remain
scalar since `mAdaptiveSmoothCoeff` is a single float and only executed when
`mParams.adaptiveRelease` is true. Insert the SIMD path for steps 3b and 4 only.

## Produces
None

## Consumes
None

## Relevant Files
Read:    `M-LIM/src/dsp/TransientLimiter.cpp` — reference SIMD implementation (lines 352–400)
Modify:  `M-LIM/src/dsp/LevelingLimiter.cpp` — add SIMD path inside the per-sample loop
Read:    `M-LIM/src/dsp/LevelingLimiter.h`    — check member layout

## Acceptance Criteria
- [ ] Run: `cmake --build build --target MLIM_Standalone_Standalone -j$(nproc)` → Expected: compiles without errors or warnings
- [ ] Run: `cd build && ctest --output-on-failure -R LevelingLimiter` → Expected: all existing tests pass

## Tests
None (performance optimization, correctness preserved by existing tests)

## Technical Details
Pattern to follow (from TransientLimiter.cpp lines 352–400):

```cpp
#if JUCE_USE_SIMD
    using SIMDf = juce::dsp::SIMDRegister<float>;
    if (chCount == 2 && SIMDf::SIMDNumElements >= 4)
    {
        alignas(...) float gArr[4]  = { mGainState[0], mGainState[1], 1.0f, 1.0f };
        alignas(...) float tArr[4]  = { perChRequiredGain[0], perChRequiredGain[1], 1.0f, 1.0f };
        SIMDf g      = SIMDf::fromRawArray(gArr);
        SIMDf target = SIMDf::fromRawArray(tArr);
        SIMDf attack  = g * SIMDf::expand(mAttackCoeff) + target * SIMDf::expand(1.0f - mAttackCoeff);
        SIMDf release = g * SIMDf::expand(mEffectiveReleaseCoeff) + ... // per-channel coeff needed
        ...
    }
#endif
```

Note: `effectiveReleaseCoeffs[ch]` is per-channel (due to adaptive release), so use a
2-element SIMD load rather than `expand()`. Clamp output to `[kMinGain, 1.0f]`.

## Dependencies
None
