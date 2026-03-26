# Task 284: TransientLimiter::computeRequiredGain() recomputes threshold dB on every sample

## Description

`TransientLimiter::computeRequiredGain()` calls `gainToDecibels(mThreshold)` on **every sample** from the inner processing loop. `mThreshold` is a constant during a block — the log10 computation is wasted work on every call.

At 32x oversampling (44.1 kHz × 32 = ~1.4 MHz) with 2 channels, this is ~2.8 million redundant `log10` calls per second for any soft-knee algorithm (all algorithms except `kneeHalf < 0.01`). Each `log10` takes ~20–50 ns on modern hardware.

Relevant path in `TransientLimiter.cpp`:

```cpp
// Called per-sample from the process() inner loop:
float TransientLimiter::computeRequiredGain(float peakAbs) const
{
    ...
    // Soft-knee path: always computes two log10 calls,
    // even when peakAbs is well below the lower knee edge.
    const float peakDb   = gainToDecibels(peakAbs);           // OK — changes per sample
    const float threshDb = gainToDecibels(mThreshold);        // REDUNDANT — constant during block
    const float lowerDb  = threshDb - kneeHalf;               // REDUNDANT — constant during block
    const float upperDb  = threshDb + kneeHalf;               // REDUNDANT — constant during block
    if (peakDb <= lowerDb)
        return 1.0f;   // exits here for below-knee signals — but log10 was already called
```

Additionally, a linear-domain lower-knee threshold (`mLinearLowerKneeThresh`) can be pre-computed so that below-knee samples can be short-circuited **before any log10 call**. In typical mastering use the signal is below the limiter knee most of the time, so this early exit eliminates nearly all log10 calls.

## Produces
None

## Consumes
None

## Relevant Files
Modify: `M-LIM/src/dsp/TransientLimiter.h` — add cached dB/linear knee threshold members
Modify: `M-LIM/src/dsp/TransientLimiter.cpp` — update cache in `setThreshold()` / `setAlgorithmParams()`, use cache in `computeRequiredGain()`

## Acceptance Criteria
- [ ] Run: `cd M-LIM/build && ctest -R transient_limiter --output-on-failure` → Expected: all transient limiter tests pass
- [ ] Run: `grep -n "gainToDecibels(mThreshold)" M-LIM/src/dsp/TransientLimiter.cpp` → Expected: no matches (threshold dB comes from cached member)
- [ ] Run: `grep -n "mThresholdDb\|mLinearLowerKnee" M-LIM/src/dsp/TransientLimiter.h` → Expected: at least two matches (the new cached members are declared)

## Tests
- Unit: Verify that after calling `setThreshold(0.891f)` (≈ −1 dBFS) and then `process()`, the output is correctly limited to ≤ 0.891 for a full-scale 1 kHz sine at all knee widths (0, 3, 6, 12 dB). This validates correctness of the cached path.
- Unit: Verify that calling `setThreshold()` mid-stream (before the second block) correctly updates the cache and the new threshold is respected from the very first sample of the next block.

## Technical Details

**Step 1** — Add cached members to `TransientLimiter.h`:

```cpp
// Cached dB/linear equivalents of the knee thresholds.
// Updated by setThreshold() and setAlgorithmParams(); used every sample by computeRequiredGain().
float mThresholdDb          = 0.0f;   // gainToDecibels(mThreshold)
float mLinearLowerKneeThresh = 1.0f;  // linear equivalent of (mThresholdDb - kneeHalf)
// Also useful: mUpperKneeDb = mThresholdDb + kneeHalf (saved to avoid per-call add)
float mLowerKneeDb = 0.0f;
float mUpperKneeDb = 0.0f;
```

**Step 2** — Add a private helper to recompute the cache whenever threshold or knee changes:

```cpp
void TransientLimiter::updateKneeCache()
{
    mThresholdDb         = gainToDecibels(mThreshold);
    const float kneeHalf = mParams.kneeWidth * 0.5f;
    mLowerKneeDb         = mThresholdDb - kneeHalf;
    mUpperKneeDb         = mThresholdDb + kneeHalf;
    mLinearLowerKneeThresh = decibelsToGain(mLowerKneeDb);
}
```

Call `updateKneeCache()` from `setThreshold()` and from `setAlgorithmParams()` (after updating `mParams`).

**Step 3** — Rewrite `computeRequiredGain()` to use the cache:

```cpp
float TransientLimiter::computeRequiredGain(float peakAbs) const
{
    if (peakAbs < kEpsilon)
        return 1.0f;

    const float kneeHalf = mParams.kneeWidth * 0.5f;

    if (kneeHalf < 0.01f)
    {
        // Hard knee — no log10 needed
        return (peakAbs > mThreshold) ? (mThreshold / peakAbs) : 1.0f;
    }

    // Linear-domain early exit: below lower knee → no reduction (no log10 call)
    if (peakAbs <= mLinearLowerKneeThresh)
        return 1.0f;

    // At this point we know we are in or above the knee — log10 is justified
    const float peakDb = gainToDecibels(peakAbs);

    if (peakDb >= mUpperKneeDb)
    {
        // Above knee — hard limit
        return mThreshold / peakAbs;
    }

    // Within knee: quadratic interpolation
    const float t      = (peakDb - mLowerKneeDb) / mParams.kneeWidth;
    const float gainDb = (mThresholdDb - mUpperKneeDb) * t * t;
    return std::pow(10.0f, gainDb / 20.0f);
}
```

Note: `decibelsToGain()` is already in `DspUtil.h`.

**Why this is safe**: `mThreshold` only changes via `setThreshold()`, `mParams.kneeWidth` only changes via `setAlgorithmParams()`. Both are called on the audio thread (via `applyPendingParams()` at the start of `LimiterEngine::process()`), so there is no threading concern for the cache variables.

## Dependencies
None
