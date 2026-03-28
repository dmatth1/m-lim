# Task: Remove Dead `kMinGaindB` Constant and Deduplicate `kMinGain` in LevelingLimiter.cpp

## Description
`LevelingLimiter.cpp` defines two file-scope constants at the top:

```cpp
static constexpr float kMinGaindB = -120.0f;  // line 8 — NEVER USED
static constexpr float kMinGain   = 1e-6f;    // line 9 — duplicates kDspUtilMinGain
```

`kMinGaindB` is defined but never referenced anywhere in the file. It is dead code that only
occupies a line and misleads readers into thinking there is dB-domain gain clamping logic.

`kMinGain` duplicates `kDspUtilMinGain` (= `1e-6f`) from `DspUtil.h`, which `LevelingLimiter.cpp`
already includes. Having two constants with the same value and slightly different names creates
maintenance risk: if the floor value ever changes in `DspUtil.h`, `LevelingLimiter.cpp` would
silently diverge.

Remove `kMinGaindB` entirely and replace the local `kMinGain` with `kDspUtilMinGain`.

## Produces
None

## Consumes
None

## Relevant Files
Modify: `M-LIM/src/dsp/LevelingLimiter.cpp` — delete lines 8–9 (`kMinGaindB` and `kMinGain`),
  replace all uses of `kMinGain` in the file with `kDspUtilMinGain`

## Acceptance Criteria
- [ ] Run: `grep -n "kMinGaindB\|kMinGain" M-LIM/src/dsp/LevelingLimiter.cpp` → Expected: no output
- [ ] Run: `cmake --build build --target MLIM_Standalone_Standalone -j$(nproc) 2>&1 | tail -3` → Expected: build succeeds
- [ ] Run: `cd build && ctest --output-on-failure` → Expected: all tests pass

## Tests
None (no behavior change)

## Technical Details
- `kMinGain` is used at 2 sites in `LevelingLimiter.cpp`: line 250 (SIMD) and line 286 (scalar).
  Both should become `kDspUtilMinGain`.
- `DspUtil.h` is already `#include`d in `LevelingLimiter.cpp` (line 2), so no new include needed.
- `kMinGaindB` is not used at all — grep confirms zero references in the file body.

## Dependencies
None
