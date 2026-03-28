# Task: Fix Scalar/SIMD Clamp Divergence in LevelingLimiter::process()

## Description
`LevelingLimiter::process()` has two code paths for gain smoothing: a SIMD path (stereo + 4-wide
SIMD) and a scalar path (mono/surround/non-SIMD). The SIMD path clamps the updated gain to
`[kMinGain, 1.0f]` on both ends:

```cpp
// SIMD path (LevelingLimiter.cpp lines 249–251)
newG = SIMDf::max(newG, SIMDf::expand(kMinGain));
newG = SIMDf::min(newG, SIMDf::expand(1.0f));   // ← upper clamp present
```

The scalar path only applies the lower clamp:

```cpp
// Scalar path (LevelingLimiter.cpp line 286)
g = std::max(g, kMinGain);                        // ← upper clamp MISSING
```

Although mathematically `g` should never exceed 1.0 given the attack/release formulas (both
bounded inputs `g` and `target` ≤ 1.0), floating-point rounding near coefficients of exactly
1.0 can push `g` slightly above 1.0 in rare edge cases. More importantly, the behavioral
divergence between SIMD and scalar paths is a latent bug: any future code change that allows
`target > 1.0` (e.g., upward expansion) would break the scalar path silently while the SIMD
path remains safe.

Add the missing upper clamp to the scalar path so both paths are identical.

## Produces
None

## Consumes
None

## Relevant Files
Modify: `M-LIM/src/dsp/LevelingLimiter.cpp` — in the scalar path of `process()`, change:
  ```cpp
  g = std::max(g, kMinGain);
  ```
  to:
  ```cpp
  g = std::clamp(g, kMinGain, 1.0f);
  ```

## Acceptance Criteria
- [ ] Run: `grep -A1 "std::max(g, kMinGain)" M-LIM/src/dsp/LevelingLimiter.cpp` → Expected: no output (replaced with clamp)
- [ ] Run: `cmake --build build --target MLIM_Standalone_Standalone -j$(nproc) 2>&1 | tail -3` → Expected: build succeeds
- [ ] Run: `cd build && ctest --output-on-failure` → Expected: all tests pass

## Tests
None (existing LevelingLimiter tests verify numerical behavior; the change is defensive only)

## Technical Details
- The fix is a single-line change: `std::max(g, kMinGain)` → `std::clamp(g, kMinGain, 1.0f)`
- `std::clamp` is available in C++17 (`<algorithm>`) which this project already targets
- `kMinGain` is defined at the top of `LevelingLimiter.cpp` as `1e-6f`
- This change makes the scalar path identical in behavior to the SIMD path's min/max pair

## Dependencies
None
