# Task 166: LevelingLimiter::process() Per-Sample Transcendental Calls in Attack/Release Paths

## Description
`LevelingLimiter::process()` calls `gainToDecibels()` (`std::log10`) and `decibelsToGain()`
(`std::pow`) inside the per-sample inner loop for both the attack and release smoothing paths.
Task 080 moved gain *application* to linear domain (line ~199), but the smoothing computation
itself still uses dB conversions:

**Attack path** (`LevelingLimiter.cpp` lines ~161–164):
```cpp
const float gDb      = gainToDecibels(g);        // std::log10 — per sample
const float targetDb = gainToDecibels(target);   // std::log10 — per sample
const float smoothedDb = gDb + (targetDb - gDb) * (1.0f - mAttackCoeff);
g = decibelsToGain(smoothedDb);                  // std::pow   — per sample
```

**Release path** (`LevelingLimiter.cpp` lines ~188–190):
```cpp
const float gDb      = gainToDecibels(g);        // std::log10 — per sample
const float smoothedDb = gDb + (0.0f - gDb) * (1.0f - effectiveReleaseCoeff);
g = decibelsToGain(smoothedDb);                  // std::pow   — per sample
```

**Adaptive release tracker** (`LevelingLimiter.cpp` lines ~152–154):
```cpp
const float targetDb = gainToDecibels(target);   // std::log10 — per sample (when adaptiveRelease)
mEnvState[ch] = mEnvState[ch] * mAdaptiveSmoothCoeff + targetDb * ...;
```

**Performance impact at high oversampling**: LevelingLimiter runs at the upsampled rate.
At 32x oversampling + stereo (2 × 32 × 44100 = 2.82 MHz effective sample rate):
- Attack path: ~5.6M `log10` + 2.8M `pow` calls/sec
- Release path: ~2.8M `log10` + 2.8M `pow` calls/sec

This is a real-time performance bottleneck that violates the project's audio-thread safety
goals (expensive non-allocating transcendentals that can still cause timing jitter).

**Root cause**: The comment at line 139–142 correctly notes that linear-domain tracking
"avoids a per-sample std::pow call" — but only applies to the *application* step (line ~199).
The smoothing steps themselves were not converted.

**Fix**: Reformulate attack, release, and adaptive tracking in linear domain:

```cpp
// Attack — linear-domain first-order IIR (equivalent for typical ms-range times)
g = g * mAttackCoeff + target * (1.0f - mAttackCoeff);

// Release — linear recovery toward 1.0 (unity)
g = g * effectiveReleaseCoeff + 1.0f * (1.0f - effectiveReleaseCoeff);
// simplified: g = g * effectiveReleaseCoeff + (1.0f - effectiveReleaseCoeff);

// Adaptive tracker — linear-domain (mEnvState stored as linear ratio, not dB)
mEnvState[ch] = mEnvState[ch] * mAdaptiveSmoothCoeff
                + target * (1.0f - mAdaptiveSmoothCoeff);
// adaptive speedup uses log-domain GR depth: compute once per block, not per sample
```

Note: `mEnvState` changing from dB to linear domain requires updating the adaptive release
speedup logic at lines ~174–183 (which reads `mEnvState[ch]` as dB). The new linear-domain
equivalent of `sustainedGRdB` is:

```cpp
// linear mEnvState: 1.0 = no reduction, <1 = reduction
// sustained reduction depth in dB:
const float sustainedGRdB = gainToDecibels(mEnvState[ch]);  // called once per channel, not per sample
```

This `gainToDecibels` call is now per-channel (2 calls per block) not per-sample — negligible.

## Produces
None

## Consumes
None

## Relevant Files
Modify: `M-LIM/src/dsp/LevelingLimiter.cpp` — replace per-sample dB conversions in `process()`
Read:   `M-LIM/src/dsp/LevelingLimiter.h` — check mEnvState type and member layout
Read:   `M-LIM/src/dsp/DspUtil.h` — `gainToDecibels` / `decibelsToGain` definitions
Read:   `M-LIM/tests/dsp/test_leveling_limiter.cpp` — existing tests that must still pass

## Acceptance Criteria
- [ ] Run: `grep -n "gainToDecibels\|decibelsToGain" M-LIM/src/dsp/LevelingLimiter.cpp` → Expected: no calls inside the per-sample `for (int s = 0; s < numSamples; ++s)` loop body (only `mCurrentGRdB` assignment outside the loop is permitted)
- [ ] Run: `cd build && cmake --build . -j$(nproc) 2>&1 | tail -5` → Expected: compiles without errors
- [ ] Run: `cd build && ctest --output-on-failure 2>&1 | tail -20` → Expected: all tests pass

## Tests
- Unit: add test in `tests/dsp/test_leveling_limiter.cpp` verifying that gain reduction during
  a constant-level tone converges to the correct threshold (< 0.5 dBTP difference from
  the dB-domain implementation) after 1000 samples at 44100 Hz, attack=10ms, release=100ms
- Unit: add test verifying release recovery from -20 dBGR to < -0.1 dBGR within
  `3 * releaseMs` samples (tests that linear-domain release speed is preserved)
- Unit: add test with `adaptiveRelease=true` verifying `mEnvState` converges correctly after
  sustained gain reduction

## Technical Details
**Equivalence argument**: For first-order IIR smoothing with coefficient `c` near 1.0
(slow time constants > 10 ms at 44.1 kHz → `c > 0.9977`), the difference between
linear-domain and dB-domain smoothing is < 0.5% per sample. The existing comment at
line 139 already documents this approximation as acceptable.

**`mEnvState` type change**: `mEnvState[ch]` must change from dB domain to linear domain.
Update `prepare()` initial value from `0.0f` (0 dB) to `1.0f` (unity gain) and
adjust the speedup condition comparison from `sustainedGRdB > 0.5f` to check the
`gainToDecibels(mEnvState[ch])` equivalent (computed once per channel per block).

**`mGainState` initial value**: Already `1.0f` (linear) — no change needed.

## Dependencies
None
