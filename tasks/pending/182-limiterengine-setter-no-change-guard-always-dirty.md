# Task 182: LimiterEngine Setters Lack Change Guards — mParamsDirty Set Every Block

## Description
`PluginProcessor::pushAllParametersToEngine()` is called every `processBlock()`, invoking
all ~15 `LimiterEngine` setter methods unconditionally. Each setter immediately calls
`mParamsDirty.store(true)` without checking whether the incoming value differs from the
stored value.

**Consequence**: `applyPendingParams()` runs on every audio-thread block — even during
steady-state operation where no parameters have changed. This triggers:
1. `mLevelingLimiter.setAttack()` → `updateCoefficients()` → 3× `std::exp()`
2. `mLevelingLimiter.setRelease()` → `updateCoefficients()` → 3× `std::exp()` (same call)
3. `mTransientLimiter.setLookahead()` (recalculates `mLookaheadSamples`)
4. `AlgorithmParams` lookup and threshold recalculation

Additionally, `setInputGain(dB)` and `setOutputCeiling(dB)` each call
`std::pow(10.0f, dB / 20.0f)` in their bodies — also executed every block.

**Locations**:
- `M-LIM/src/dsp/LimiterEngine.cpp` — all `setXxx()` methods (~lines 355–467)
- `M-LIM/src/dsp/LevelingLimiter.cpp` — `setAttack()` line 58, `setRelease()` line 67

**Root cause**: No guard to short-circuit when the new value equals the stored value.

**Fix**: Add change guards to each `LimiterEngine` setter:
```cpp
void LimiterEngine::setInputGain(float dB)
{
    const float newLinear = std::pow(10.0f, dB / 20.0f);
    if (newLinear == mInputGainLinear.load(std::memory_order_relaxed))
        return;
    mInputGainLinear.store(newLinear);
    mParamsDirty.store(true);
}
```

For floating-point equality comparison, bit-exact comparison (as used in `SidechainFilter`)
or a small epsilon check are both acceptable.

Add change guards to `LevelingLimiter::setAttack()` and `setRelease()` so
`updateCoefficients()` only runs when the value actually changes.

## Produces
None

## Consumes
None

## Relevant Files
Modify: `M-LIM/src/dsp/LimiterEngine.cpp` — add change guards to all setter methods
Modify: `M-LIM/src/dsp/LevelingLimiter.cpp` — guard `setAttack()`, `setRelease()`
Read:   `M-LIM/src/dsp/LevelingLimiter.h` — check stored member types (`mAttackMs`, `mReleaseMs`)
Read:   `M-LIM/src/dsp/SidechainFilter.cpp` — reference implementation of `floatBitsEqual` guard
Read:   `M-LIM/src/PluginProcessor.cpp` — `pushAllParametersToEngine()` call pattern

## Acceptance Criteria
- [ ] Run: `cd build && cmake --build . -j$(nproc) 2>&1 | tail -5` → Expected: compiles without errors
- [ ] Run: `cd build && ctest --output-on-failure 2>&1 | tail -20` → Expected: all tests pass
- [ ] Run: script that calls `pushAllParametersToEngine()` twice with identical params and verifies `mParamsDirty` is false on the second call → Expected: dirty flag not set when values unchanged

## Tests
- Unit: add test in `tests/dsp/test_limiter_engine.cpp` verifying that calling the same setter
  twice with the same value does NOT set `mParamsDirty` the second time (expose via a
  `getParamsDirty()` getter or test indirectly via checking `applyPendingParams` side-effects)
- Unit: verify `setAttack(100.0f)` followed by `setAttack(100.0f)` does not call
  `updateCoefficients()` a second time (use a counter or check coefficients are stable)

## Technical Details
- Use `floatBitsEqual()` (already in `SidechainFilter.cpp`) or inline it in a shared header
  for bit-exact float comparison — avoids `-Wfloat-equal` warnings
- For `setInputGain`/`setOutputCeiling`: compare the computed linear value against stored
  atomic, OR compare the dB input against a separate cached dB member
- `LevelingLimiter::setAttack()` and `setRelease()` store `mAttackMs` / `mReleaseMs` as
  plain floats — add `if (ms == mAttackMs) return;` guard (bit-exact or epsilon)
- Do NOT add a guard to `setOversamplingFactor()` — it already has its own change detection
  path via `mCurrentOversamplingFactor` in `applyPendingParams()`

## Dependencies
Requires task 178 (both modify LevelingLimiter.cpp — transcendental fix lands first)
