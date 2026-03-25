# Task 171: Extract Duplicated Threshold Clamping into DspUtil

## Description
`TransientLimiter::setThreshold()` and `LevelingLimiter::setThreshold()` contain identical clamping logic:

```cpp
// TransientLimiter.cpp line 69
mThreshold = std::clamp(linear, 1e-6f, 1.0f);

// LevelingLimiter.cpp line 52
mThreshold = std::clamp(linear, 1e-6f, 1.0f);
```

The magic value `1e-6f` is also independently duplicated — it is the same as `kDspUtilMinGain` already declared in `DspUtil.h`. This means the floor value exists in three places.

Add a `clampThreshold(float linear)` helper to `DspUtil.h` that enforces the `[kDspUtilMinGain, 1.0f]` range, and replace both call sites.

## Produces
None

## Consumes
None

## Relevant Files
Modify: `M-LIM/src/dsp/DspUtil.h` — add `clampThreshold()` inline helper
Modify: `M-LIM/src/dsp/TransientLimiter.cpp` — use `clampThreshold()`
Modify: `M-LIM/src/dsp/LevelingLimiter.cpp` — use `clampThreshold()`
Read: `M-LIM/src/dsp/DspUtil.h` — see existing style and `kDspUtilMinGain`

## Acceptance Criteria
- [ ] Run: `grep -n "std::clamp(linear, 1e-6f" M-LIM/src/dsp/TransientLimiter.cpp M-LIM/src/dsp/LevelingLimiter.cpp` → Expected: no matches
- [ ] Run: `cmake --build build -j$(nproc) 2>&1 | tail -5` → Expected: build succeeds, exit 0
- [ ] Run: `cd build && ctest --output-on-failure -R "test_transient_limiter|test_leveling_limiter" 2>&1 | tail -10` → Expected: all tests pass

## Tests
None

## Technical Details
Add to `DspUtil.h` after the existing helpers:
```cpp
// Clamps a linear threshold gain value to a safe [kDspUtilMinGain, 1.0f] range.
// kDspUtilMinGain (1e-6f) prevents division-by-zero in gain calculations.
inline float clampThreshold(float linear) noexcept
{
    return std::clamp(linear, kDspUtilMinGain, 1.0f);
}
```

In both limiter `setThreshold()` implementations replace:
```cpp
mThreshold = std::clamp(linear, 1e-6f, 1.0f);
```
with:
```cpp
mThreshold = clampThreshold(linear);
```

Include `DspUtil.h` in `LevelingLimiter.cpp` and `TransientLimiter.cpp` if not already included.

## Dependencies
None
