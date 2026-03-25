# Task 119: LevelingLimiter — Release/Attack Smoothing in Linear Domain Produces Non-Exponential-in-dB Envelope

## Description

`LevelingLimiter::process()` (lines 181–213) applies attack and release smoothing in the **linear gain domain** using coefficients that were computed for **dB-domain** operation:

```cpp
// Attack
g = g * mAttackCoeff + target * (1.0f - mAttackCoeff);

// Release
g = g * effectiveReleaseCoeff + (1.0f - effectiveReleaseCoeff);
```

`mAttackCoeff` and `mReleaseCoeff` are computed as:
```cpp
mReleaseCoeff = std::exp(-1.0f / (mReleaseMs * 0.001f * sr));
```

This formula produces the correct coefficient when used in a dB-domain first-order smoother, where the state variable is gain **in dB**. When the same coefficient is applied to the **linear** gain state, the envelope shape in dB is non-exponential.

**Quantitative impact (Release from -20 dB, release time τ = 100 samples):**

After one time constant (τ samples):

| Domain | State at τ | dB at τ | Interpretation |
|--------|-----------|---------|----------------|
| Linear (as coded) | g = 0.669 | **−3.49 dB** | Covers 82.5% of 20 dB in one τ |
| dB (correct) | gDb = −7.36 dB | **−7.36 dB** | Covers 63.2% of 20 dB in one τ |

The linear-domain implementation jumps from −20 dB to −3.49 dB in one time constant, making the initial release approximately **2× faster in dB/ms** than specified. The remaining 3.5 dB then takes disproportionately long to recover, creating an audible "snap followed by a lingering tail" artifact — especially noticeable on heavily limited programme material (sustained gain reduction > 6 dB).

**By contrast**, `TransientLimiter` (Stage 1) correctly performs release smoothing in the dB domain (lines 295–299):

```cpp
const float gDb      = gainToDecibels(g);
const float targetDb = gainToDecibels(target);
const float smoothedDb = gDb + (targetDb - gDb) * (1.0f - mReleaseCoeff);
g = decibelsToGain(smoothedDb);
```

`LevelingLimiter` must be brought into alignment with this approach.

**Fix:** In `LevelingLimiter::process()`, replace the linear-domain interpolation for both attack and release with dB-domain interpolation matching the Stage 1 implementation. The `mGainState` array stays in linear scale at all other points (only the smoothing step moves to dB); or it can be stored in dB throughout this function call and converted at the application step. The existing `gainToDecibels()`/`decibelsToGain()` helpers already exist in the file.

## Produces
None

## Consumes
None

## Relevant Files
Modify: `M-LIM/src/dsp/LevelingLimiter.cpp` — fix attack/release smoothing in process() (lines 181–213)
Read: `M-LIM/src/dsp/TransientLimiter.cpp` — reference implementation of correct dB-domain smoothing (lines 284–302)
Read: `M-LIM/src/dsp/LevelingLimiter.h` — verify mGainState and coefficient member layout
Read: `M-LIM/tests/dsp/test_leveling_limiter.cpp` — existing tests to ensure they still pass

## Acceptance Criteria
- [ ] Run: `cd /workspace/M-LIM/build && ctest -R LevelingLimiter --output-on-failure` → Expected: all tests pass, exit 0
- [ ] Run: `cd /workspace/M-LIM/build && ctest --output-on-failure` → Expected: all tests pass, exit 0
- [ ] Run: Verify by code review that `LevelingLimiter::process()` calls `gainToDecibels()` and `decibelsToGain()` on the smoothing state path (not only the adaptive-release branch)

## Tests
- Unit: `tests/dsp/test_leveling_limiter.cpp::test_release_is_exponential_in_dB` — apply a +12 dB impulse to drive GR to -12 dB, then verify that the gain-in-dB rises at a constant rate per sample during release (each 100-sample interval in the release tail must cover the same ±0.3 dB range, within 10% tolerance)
- Unit: `tests/dsp/test_leveling_limiter.cpp::test_release_time_constant_correct` — with a 100 ms release at 44100 Hz, verify the gain recovers to within 1/e of the target after 4410 samples (tolerance ±5%)
- Unit: `tests/dsp/test_leveling_limiter.cpp::test_attack_time_constant_correct` — with a 10 ms attack at 44100 Hz, verify that 63.2% of the required gain reduction is applied within 441 samples

## Technical Details

**Corrected release block** (replace lines 191–211 in process()):

```cpp
if (target < g)
{
    // Attack: instant approach in dB domain (same as TransientLimiter)
    const float gDb      = gainToDecibels(g);
    const float targetDb = gainToDecibels(target);
    const float smoothedDb = gDb + (targetDb - gDb) * (1.0f - mAttackCoeff);
    g = decibelsToGain(smoothedDb);
}
else
{
    // Release: recover toward unity in dB domain
    float effectiveReleaseCoeff = mReleaseCoeff;
    if (mParams.adaptiveRelease)
    {
        const float sustainedGRdB = -mEnvState[ch];
        if (sustainedGRdB > 0.5f)
        {
            const float speedup   = std::min(1.0f, sustainedGRdB / 6.0f);
            const float fastCoeff = mReleaseCoeff * mReleaseCoeff;
            effectiveReleaseCoeff = mReleaseCoeff * (1.0f - speedup) + fastCoeff * speedup;
        }
    }
    const float gDb      = gainToDecibels(g);
    const float smoothedDb = gDb + (0.0f - gDb) * (1.0f - effectiveReleaseCoeff); // target = 0 dB
    g = decibelsToGain(smoothedDb);
}
g = std::max(g, kMinGain);
```

The adaptive-release `mEnvState` tracking can remain in the dB domain as-is (it already calls `gainToDecibels(target)`).

## Dependencies
None
