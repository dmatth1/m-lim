# Task 060: Limiter Threshold Should Track Output Ceiling Instead of Hard Clipping

## Description
Both `TransientLimiter` and `LevelingLimiter` have their threshold hardcoded to 1.0 (0 dBFS). The output ceiling is applied as a hard clip in `LimiterEngine::process()` step 7:

```cpp
data[i] = std::max(-ceiling, std::min(ceiling, data[i]));
```

This means when the ceiling is set below 0 dBFS (e.g., -0.3 dBFS = 0.966 linear, or -1.0 dBFS = 0.891 linear), the limiter allows peaks through up to 0 dBFS, and then the hard clip at the ceiling introduces distortion. The limiter's smooth gain reduction is wasted for the range between ceiling and 0 dBFS.

**Correct behavior**: The limiter threshold should match the output ceiling. The limiters should smoothly reduce gain to keep the signal below the ceiling, and the hard clip at step 7 should only be a safety net (catching rare overshoots), not the primary level control.

At -1.0 dBFS ceiling, the current code hard-clips ~0.9 dB of signal range. This is audible as distortion on transients — exactly what a limiter is supposed to prevent.

## Produces
None

## Consumes
None

## Relevant Files
Modify: `M-LIM/src/dsp/TransientLimiter.h` — add setThreshold() method or make threshold a parameter
Modify: `M-LIM/src/dsp/TransientLimiter.cpp` — use mThreshold in computeRequiredGain (already does, but threshold is never updated from 1.0)
Modify: `M-LIM/src/dsp/LevelingLimiter.h` — add setThreshold() method
Modify: `M-LIM/src/dsp/LevelingLimiter.cpp` — use mThreshold (already does, but never updated)
Modify: `M-LIM/src/dsp/LimiterEngine.cpp` — pass ceiling to both limiters as threshold

## Acceptance Criteria
- [ ] Run: `cd build && ctest --output-on-failure` → Expected: all tests pass
- [ ] Run: Test that with ceiling set to -1.0 dBFS, output peaks stay below ceiling without hard clipping (verify gain reduction is applied smoothly, not via clip)

## Tests
- Unit: `tests/dsp/test_limiter_engine.cpp::test_ceiling_below_zero_dbfs` — set ceiling to -1.0 dBFS, process a 0 dBFS sine wave, verify output stays below ceiling AND that the output is smooth (no hard clip artifacts — compare with a properly limited reference)
- Unit: `tests/dsp/test_transient_limiter.cpp::test_custom_threshold` — set threshold to 0.5 and verify gain reduction activates at the correct level

## Technical Details
1. Add `void setThreshold(float linear)` to both `TransientLimiter` and `LevelingLimiter`. The `mThreshold` member already exists and is used by `computeRequiredGain()` — it just needs a public setter.

2. In `LimiterEngine::prepare()` and `applyPendingParams()`, after reading the ceiling value, call:
```cpp
const float ceiling = mOutputCeilingLinear.load();
mTransientLimiter.setThreshold(ceiling);
mLevelingLimiter.setThreshold(ceiling);
```

3. The hard clip at step 7 should remain as a safety net but should rarely activate (only for samples that slip through the lookahead window).

4. For unity gain mode, the threshold should be `1.0 / inputGain` (matching the existing ceiling calculation).

## Dependencies
None
