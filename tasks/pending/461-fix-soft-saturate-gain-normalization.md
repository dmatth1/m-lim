# Task 461: Fix softSaturate Gain Normalization in TransientLimiter

## Description
The `softSaturate()` function in `TransientLimiter.cpp` (line 205-213) uses `tanh(x * drive) / drive` as the wet signal, but `/drive` is incorrect normalization for gain-preserving saturation. This causes signals near full scale (e.g., 0.9) to be attenuated to ~0.25 when `amount = 1.0`, effectively adding ~11 dB of unintended gain reduction on top of the limiter's actual gain reduction.

**The bug**: For `amount = 0.8` (Aggressive algorithm), a post-limiting signal at 0.9 becomes ~0.41 — nearly -7 dB of extra gain reduction from saturation alone. This is not harmonic coloring; it's level destruction.

**Correct formula**: `tanh(x * drive) / tanh(drive)` normalizes so that `f(1.0) = 1.0` (or close), preserving signal level while adding harmonic saturation. Alternatively, a common approach is `tanh(x * drive) * (1.0 / tanh(drive))` where the normalization constant can be precomputed per parameter change.

**Current code** (`TransientLimiter.cpp:211-213`):
```cpp
const float drive = 1.0f + amount * 3.0f;
const float wet   = std::tanh(x * drive) / drive;
return x * (1.0f - amount) + wet * amount;
```

**Correct code**:
```cpp
const float drive = 1.0f + amount * 3.0f;
const float normalization = 1.0f / std::tanh(drive);  // precompute if possible
const float wet = std::tanh(x * drive) * normalization;
return x * (1.0f - amount) + wet * amount;
```

Note: The normalization constant `1.0f / std::tanh(drive)` depends only on `drive` (which depends on `amount` = `saturationAmount` from AlgorithmParams), so it could be precomputed in `setAlgorithmParams()` and cached, avoiding a per-sample `tanh()` call on the normalization.

## Produces
None

## Consumes
None

## Relevant Files
Modify: `M-LIM/src/dsp/TransientLimiter.cpp` — fix softSaturate() normalization (line 211)
Read: `M-LIM/src/dsp/TransientLimiter.h` — may want to cache normalization constant as member
Read: `M-LIM/src/dsp/LimiterAlgorithm.h` — saturationAmount values per algorithm

## Acceptance Criteria
- [ ] Run: `cd build && ctest --output-on-failure` → Expected: all tests pass, exit 0
- [ ] Run: `python3 -c "import math; drive=4.0; x=0.9; print(f'old={math.tanh(x*drive)/drive:.4f} new={math.tanh(x*drive)/math.tanh(drive):.4f}')"` → Expected: old=0.2498 new=0.9993 (showing the fix preserves signal level)

## Tests
- Unit: `tests/dsp/test_transient_limiter.cpp::test_soft_saturate_unity_gain` — verify that softSaturate(1.0, amount) returns ~1.0 for all amount values
- Unit: `tests/dsp/test_transient_limiter.cpp::test_soft_saturate_small_signal` — verify small signals pass through unchanged

## Technical Details
- The fix is a one-line change to the normalization divisor
- Consider precomputing `1.0f / std::tanh(drive)` in setAlgorithmParams() to avoid per-sample overhead
- This affects all algorithms with saturationAmount > 0: Punchy(0.3), Dynamic(0.2), Aggressive(0.8), Allround(0.4), Bus(0.7), Modern(0.1)

## Dependencies
None
