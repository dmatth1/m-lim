# Task 272: AlgorithmParams::transientAttackCoeff and dynamicEnhance are dead code — never applied in DSP

## Description

`AlgorithmParams` declares two fields that are never read in any DSP processing path:

```cpp
// LimiterAlgorithm.h:23–27
struct AlgorithmParams {
    float transientAttackCoeff; // how aggressively transients are caught (0-1)  ← UNUSED
    float releaseShape;
    float saturationAmount;
    float dynamicEnhance;       // transient enhancement before limiting (0-1)   ← UNUSED
    float kneeWidth;
    bool  adaptiveRelease;
};
```

A grep of all DSP source files confirms neither field is read anywhere in:
- `TransientLimiter.cpp` — attack is always instant (`g = target`)
- `LevelingLimiter.cpp` — does not use these fields
- `LimiterEngine.cpp` — no dynamic-enhancement step in the DSP chain

This means selecting algorithm "Aggressive" (`transientAttackCoeff=0.95`) vs "Transparent" (`transientAttackCoeff=0.3`) produces **identical transient-attack behavior**. The documented intent ("how aggressively transients are caught") is not implemented. Similarly, `dynamicEnhance` ("transient enhancement before limiting") is never applied.

Tests in `tests/dsp/test_limiter_algorithm.cpp` validate field range only (e.g., `p.transientAttackCoeff >= 0.0f`), not that different values produce different audio behavior.

## Produces
None

## Consumes
None

## Relevant Files
Read: `M-LIM/src/dsp/LimiterAlgorithm.h` — struct declaration with dead fields
Modify: `M-LIM/src/dsp/TransientLimiter.cpp` — `process()` step 4 (instant attack only, line ~338); implement `transientAttackCoeff`
Read: `M-LIM/src/dsp/LimiterEngine.cpp` — DSP chain steps, no dynamic enhancement step
Read: `M-LIM/tests/dsp/test_limiter_algorithm.cpp` — existing range-only tests for these fields

## Acceptance Criteria
- [ ] Run: `cd M-LIM/build && ctest -R TransientLimiter --output-on-failure` → Expected: all TransientLimiter tests pass
- [ ] Run: `cd M-LIM/build && ctest -R test_limiter_algorithm --output-on-failure` → Expected: all algorithm tests pass
- [ ] Manual verification: with a 1kHz sine wave burst test, "Aggressive" algorithm (transientAttackCoeff=0.95) and "Transparent" (transientAttackCoeff=0.3) must produce measurably different gain reduction onset behavior

## Tests
- Unit: Write a test that feeds a fast transient pulse through two `TransientLimiter` instances — one with `transientAttackCoeff=0.2` and one with `transientAttackCoeff=0.95` — and asserts that the instance with higher coefficient applies gain reduction more quickly (within fewer samples of onset).

## Technical Details

**Implementing `transientAttackCoeff` in `TransientLimiter::process()`:**

The current attack is always instant: `g = target`. To make `transientAttackCoeff` meaningful, replace the instant-attack branch with a one-pole IIR:

```cpp
// Current (instant attack):
if (target < g)
    g = target;

// With transientAttackCoeff (0 = slow/IIR, 1 = instant):
if (target < g)
{
    if (mParams.transientAttackCoeff >= 0.999f)
    {
        g = target;  // instant at coefficient = 1
    }
    else
    {
        // Map 0→1 to slow→fast attack coeff: alpha = 1 - (1 - transientAttackCoeff)^2
        // At transientAttackCoeff=0: very slow IIR; at 1: instant snap
        const float alpha = mParams.transientAttackCoeff;
        g = g * (1.0f - alpha) + target * alpha;
    }
}
```

For `dynamicEnhance`: this could pre-emphasize transients by applying a fast peak boost before the limiter sees the signal. This is more complex and may warrant a separate task. For this task, focus on `transientAttackCoeff` only, and either:
a) Remove `dynamicEnhance` from `AlgorithmParams` to eliminate dead code, or
b) Leave it declared for a future task but add a `// TODO: not yet implemented` comment.

The simplest correct fix is to implement `transientAttackCoeff` as shown above and add a `// TODO` on `dynamicEnhance`.

## Dependencies
None
