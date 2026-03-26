# Task 305: SIMD Stereo Gain Smoothing in TransientLimiter Hot Path

## Description
The gain-smoothing loop in `TransientLimiter::process()` processes stereo channel gain
states sequentially in the per-sample inner loop. At 32× oversampling (192 kHz × 32 = 6.14 M
samples/sec), this loop executes approximately 12 M gain-state updates per second (2 channels).
Vectorizing the per-channel gain computation with 4-wide float SIMD would halve or quarter
this cost.

Current code in `TransientLimiter.cpp:346–371` (gain smoothing step):
```cpp
for (int ch = 0; ch < chCount; ++ch)
{
    float& g = mGainState[ch];
    const float target = perChRequiredGain[ch];

    if (target < g)
    {
        if (mParams.transientAttackCoeff >= 0.999f)
            g = target;
        else
        {
            const float alpha = mParams.transientAttackCoeff;
            g = g * (1.0f - alpha) + target * alpha;
        }
    }
    else
    {
        g = g * mReleaseCoeff + target * (1.0f - mReleaseCoeff);
    }
    g = std::clamp(g, kDspUtilMinGain, 1.0f);
}
```

For stereo (chCount == 2), both channels can be processed in a single SSE/NEON 2-float
operation by vectorizing the IIR update and using a branchless min/blend for the
attack-vs-release selection.

Branchless formulation:
```
attackGain   = g * (1 - alpha) + target * alpha
releaseGain  = g * releaseCoeff + target * (1 - releaseCoeff)
useAttack    = target < g  (float mask)
newG         = blend(releaseGain, attackGain, useAttack)
newG         = clamp(newG, kMinGain, 1.0f)
```

This eliminates the branch and allows processing both channels in one 2-wide SIMD op
(or one 4-wide op by including all 4 SIMD elements across the attack/release branches).

**Note**: The instant-attack branch (`transientAttackCoeff >= 0.999f`) sets `g = target`
immediately. This can be handled branchlessly as: when alpha ≥ 0.999, attackGain = target
(since (1-0.999)*g + 0.999*target ≈ target). The existing check can remain as a fast-path
pre-check before entering the SIMD loop.

**Correctness**: The smoothed gain values must be numerically identical (within float epsilon)
to the scalar path. Verify with the existing `test_transient_limiter.cpp` tests.

## Produces
None

## Consumes
None

## Relevant Files
Modify: `src/dsp/TransientLimiter.cpp` — vectorize the per-channel gain smoothing step (Step 4)
Read: `src/dsp/DspUtil.h` — kDspUtilMinGain constant used in the clamp
Read: `tests/dsp/test_transient_limiter.cpp` — correctness tests that must still pass

## Acceptance Criteria
- [ ] Run: `cd /workspace/M-LIM/build && ctest -R transient --output-on-failure` → Expected: all transient limiter tests pass
- [ ] Run: `cd /workspace/M-LIM/build && ctest --output-on-failure` → Expected: all tests pass, exit 0
- [ ] Code review: the per-channel gain smoothing inner loop must use SIMD or branchless arithmetic for the stereo case

## Tests
- Unit: `tests/dsp/test_transient_limiter.cpp` — all existing gain reduction and output level tests must pass unchanged
- Unit: add `test_simd_gain_matches_scalar` that feeds the same impulsive signal to two TransientLimiter instances (one patched to force SIMD path, one using scalar) and verifies output samples agree within 1e-6f

## Technical Details
- Use `juce::dsp::SIMDRegister<float>` (SSE/NEON 4-wide) or `__m128` directly
- Only activate SIMD path when `chCount == 2` (stereo); fall through to scalar for mono/surround
- The `perChRequiredGain[ch]` array (stack-allocated, max 8 elements) must remain as-is for the sliding-window peak detection step above; only the smoothing step needs SIMD
- The existing `applyChannelLinking()` in `DspUtil.h` already runs before the smoothing step and is not part of this task

## Dependencies
None
