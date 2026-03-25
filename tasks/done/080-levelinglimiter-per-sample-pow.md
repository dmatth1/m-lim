# Task 080: LevelingLimiter::process() Calls std::pow Per Sample in Hot Path

## Description
`LevelingLimiter::process()` calls `decibelsToGain(mGainState[ch])` on every
audio sample (line ~214 in `LevelingLimiter.cpp`):

```cpp
const float linearGain = decibelsToGain(mGainState[ch]);
channelData[ch][s] *= linearGain;
```

`decibelsToGain` is:
```cpp
static inline float decibelsToGain(float dB) {
    return std::pow(10.0f, dB * (1.0f / 20.0f));
}
```

`std::pow` is a transcendental function — on x86 it typically compiles to
`pow` or uses an FPU logarithm. At 48kHz stereo with 512-sample blocks this is
~96,000 `pow` calls per second from this one function alone. Profiling
consistently shows `pow` dominates CPU in limiter plugins.

**Fix**: Track gain in the linear domain inside the process loop. The gain state
is currently stored in dB (`mGainState[ch]` in dB), but the dB-domain smoothing
formula `gDb + (targetDb - gDb) * (1 - coeff)` can be rewritten in linear domain:
`g * coeff + target * (1 - coeff)` (exponential interpolation), which is
equivalent when the release coefficient is applied consistently. The LevelingLimiter
release recovers toward 0 dB (linear 1.0), so:

```cpp
// Release in linear domain: g → 1.0 exponentially
const float target = perChRequiredGain[ch]; // already linear
g = g * releaseCoeff + target * (1.0f - releaseCoeff); // when target > g
```

Attack (target < g) stays as instant attack (g = target).

If dB-domain smoothing is required for perceptual uniformity, the gain state
can be kept in dB but converted to linear once *per channel per block* by
computing a `exp2(gDb / 6.0206f)` approximation (or a polynomial approximation
of pow10) rather than per sample.

## Produces
None

## Consumes
None

## Relevant Files
Modify: `M-LIM/src/dsp/LevelingLimiter.cpp` — refactor the inner sample loop
  to avoid per-sample `std::pow`.
Modify: `M-LIM/src/dsp/LevelingLimiter.h` — update `mGainState` comment if the
  domain changes (e.g. from dB to linear).
Read: `M-LIM/tests/dsp/test_leveling_limiter.cpp` — existing tests must pass
  after the change.

## Acceptance Criteria
- [ ] Run: `grep -n "decibelsToGain\|std::pow\|pow10" M-LIM/src/dsp/LevelingLimiter.cpp` → Expected: no `decibelsToGain` or `std::pow` calls inside the per-sample loop (the function may still be used outside the loop for non-hot-path conversions).
- [ ] Run: `cd M-LIM && ctest --test-dir build -R test_leveling_limiter --output-on-failure` → Expected: all tests pass.
- [ ] Run: `cd M-LIM && ctest --test-dir build -R test_limiter_engine --output-on-failure` → Expected: all tests pass.

## Tests
- Unit: `M-LIM/tests/dsp/test_leveling_limiter.cpp` — all existing tests must pass without modification.
- Unit: verify gain reduction is numerically close to the original dB-domain version (within 0.1 dB tolerance at the same input level).

## Technical Details
The smoothing in dB domain:
```
gDb += (targetDb - gDb) * (1 - coeff)
```
is equivalent in linear domain when the target is 1.0 (0 dB, fully open):
```
g = g + (1.0 - g) * (1 - coeff)    →    g = g * coeff + (1 - coeff)
```
When target < 1.0 (gain reduction needed):
```
g = g * coeff + target * (1 - coeff)
```
This is numerically equivalent to the dB formula when the ratio of
consecutive samples is small (which is always true for release times > 10 ms).

## Dependencies
None
