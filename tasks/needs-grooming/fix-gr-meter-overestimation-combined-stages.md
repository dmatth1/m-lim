# Fix GR Meter Overestimation from Summed Per-Block Stage Minimums

## Description

In `LimiterEngine::process()` (line 237), total gain reduction for the meter is computed as:

```cpp
const float totalGR = juce::jmax(mTransientLimiter.getGainReduction() + mLevelingLimiter.getGainReduction(), -60.0f);
```

Each `getGainReduction()` call returns the **per-block minimum** gain state in dB for that stage. Summing these minimums is mathematically incorrect when the minimums occur at different samples within the same block.

**The bug**: `min(g1[s] * g2[s]) >= min(g1[s]) * min(g2[s])` (by the product inequality for values in (0,1]). Converting to dB, the displayed GR is always **more negative** (larger in magnitude) than the actual worst-case combined gain reduction when the two stage minimums are not coincident.

**Concrete example** (1024-sample block):
- Stage 1 catches a transient at sample 10: `gain1[10] = 0.5` (-6.02 dB). Stage 1 min = 0.5.
- Stage 2 applies slow release envelope on sustained content: `gain2[200] = 0.9` (-0.92 dB). Stage 2 min = 0.9.
- At sample 10: actual combined = 0.5 × ~1.0 ≈ 0.5 (Stage 2 barely active).
- At sample 200: actual combined = ~0.95 × 0.9 ≈ 0.855 (Stage 1 mostly released).
- Actual worst combined gain = **~0.5 = -6.02 dB**
- Displayed total = -6.02 + (-0.92) = **-6.94 dB** (0.92 dB overestimate)

This causes the GR meter to consistently read higher than actual, which is misleading for mastering engineers relying on accurate GR display.

**Fix**: Track the per-sample combined gain product `g1[s] * g2[s]` in `LimiterEngine` and derive total GR from the minimum of that product across the block. This can be done in `stepRunLimiters()` or by adding a `getMinGainLinear()` method to each stage so LimiterEngine can compute the product.

The cleanest approach: add `float getMinGainLinear() const` to both limiters (returns the linear minimum gain for the last processed block, before converting to dB). Then in LimiterEngine:

```cpp
const float combinedMin = mTransientLimiter.getMinGainLinear() * mLevelingLimiter.getMinGainLinear();
const float totalGR = juce::jmax(gainToDecibels(combinedMin), -60.0f);
```

## Produces
None

## Consumes
None

## Relevant Files
Read: `M-LIM/src/dsp/LimiterEngine.cpp` — line 237: the incorrect GR summation
Modify: `M-LIM/src/dsp/LimiterEngine.cpp` — replace summed GR with product-based computation
Modify: `M-LIM/src/dsp/TransientLimiter.h` — add `getMinGainLinear() const` declaration
Modify: `M-LIM/src/dsp/TransientLimiter.cpp` — implement `getMinGainLinear()` (return `mCurrentMinGainLinear`)
Modify: `M-LIM/src/dsp/LevelingLimiter.h` — add `getMinGainLinear() const` declaration
Modify: `M-LIM/src/dsp/LevelingLimiter.cpp` — implement `getMinGainLinear()` (return `mCurrentMinGainLinear`); rename `minGain` tracking variable for clarity
Read: `M-LIM/src/dsp/DspUtil.h` — for `gainToDecibels()` helper

## Acceptance Criteria
- [ ] Run: `cd /workspace/M-LIM/build && ctest --output-on-failure` → Expected: all tests pass, exit 0
- [ ] Run: `python3 -c "import math; g1=0.5; g2=0.9; print(f'old={20*math.log10(g1)+20*math.log10(g2):.3f} dB new={20*math.log10(g1*g2):.3f} dB')"`  → Expected: old=-6.938 dB new=-6.938 dB (same here since min is at same sample — verify output changes when minimums differ)

## Tests
- Unit: `tests/dsp/test_limiter_engine.cpp::test_gr_meter_does_not_overestimate_combined_stages` — process a block where Stage 1 GR minimum and Stage 2 GR minimum are provably non-coincident (e.g., use a limiter with fast attack/slow release so Stage 1 catches a transient early and Stage 2 provides lingering envelope). Verify that `getGainReduction()` is no more negative than `gainToDecibels(Stage1.minGainLinear * Stage2.minGainLinear)`.

## Technical Details
- `mCurrentMinGainLinear` should be updated in the same place `mCurrentGRdB` is updated in each limiter (at the end of `process()`).
- The existing `mCurrentGRdB` member can remain for internal use or be removed; `LimiterEngine` should use the linear minimum to compute displayed GR.
- The `gainToDecibels()` clamp in each limiter's `getGainReduction()` call (`std::max(minGain, kDspUtilMinGain)`) should also be preserved in the new combined path to avoid -inf dB.

## Dependencies
None
