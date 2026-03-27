# Task 436: Add SIMD Stereo Fast Path to LevelingLimiter

## Description
`LevelingLimiter::process()` has no SIMD optimization for the stereo per-sample gain
smoothing loop. At 32x oversampling (1,411,200 Hz effective rate) this loop executes
~1.4M iterations/second — a hot path.

`TransientLimiter.cpp` (lines 352–400) already has a `JUCE_USE_SIMD`-guarded stereo
SIMD path. `LevelingLimiter` should follow the same pattern for steps 3b and 4 of its
inner loop (lines 180–235 in `LevelingLimiter.cpp`).

The approach:
1. Load `mGainState[0]` and `mGainState[1]` into SIMD lanes 0–1 (lanes 2–3 = 1.0f)
2. Load `perChRequiredGain[0]` and `perChRequiredGain[1]` into target lanes
3. Compute attack/release gains for both channels simultaneously using `SIMDRegister<float>`
4. Select attack/release per-lane with `SIMDf::lessThan` mask (branchless)
5. Clamp to `[kMinGain, 1.0f]`
6. Apply gain to both channels

Use `#if JUCE_USE_SIMD` guard, keeping the existing scalar path as fallback for mono/surround.

Note: `mEnvState` adaptive smoother update (step 3a, lines ~205–210) must remain scalar
since it uses a single `mAdaptiveSmoothCoeff` float. Add SIMD only for steps 3b and 4.

## Produces
None

## Consumes
None

## Relevant Files
Modify: `src/dsp/LevelingLimiter.cpp` — add `#if JUCE_USE_SIMD` fast path in process() (~lines 180–235)
Read: `src/dsp/TransientLimiter.cpp` — reference implementation of the SIMD stereo pattern (~lines 352–400)
Read: `src/dsp/LevelingLimiter.h` — `mGainState`, `mEnvState`, `mParams` field layout

## Acceptance Criteria
- [ ] Run: `cmake --build build --target MLIM_Standalone_Standalone -j$(nproc) 2>&1 | tail -3` → Expected: build succeeds, exit 0
- [ ] Run: `grep -n "JUCE_USE_SIMD" src/dsp/LevelingLimiter.cpp` → Expected: at least one occurrence of the SIMD guard

## Tests
None

## Technical Details
Follow the TransientLimiter pattern exactly. The SIMD path should:
- Be gated by `#if JUCE_USE_SIMD && (numChannels == 2)` check at runtime
- Fall through to the existing scalar loop for mono or surround configs

Build Standalone: `cmake --build build --target MLIM_Standalone_Standalone -j$(nproc)`.

## Dependencies
None
