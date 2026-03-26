# Task 271: LoudnessMeter K-weighting biquad double-precision denormal CPU spike on silence

## Description

`LoudnessMeter::Biquad::process()` uses `double z1, z2` as IIR state variables:

```cpp
// LoudnessMeter.h:67–78
struct Biquad {
    double b0, b1, b2, a1, a2;
    double z1 = 0.0, z2 = 0.0;

    float process(float x) noexcept {
        double xd = static_cast<double>(x);
        double y  = b0 * xd + z1;
        z1 = b1 * xd - a1 * y + z2;
        z2 = b2 * xd - a2 * y;
        return static_cast<float>(y);
    }
};
```

`LoudnessMeter::processBlock()` wraps with `juce::ScopedNoDenormals`. However, `ScopedNoDenormals` sets the SSE `FTZ`/`DAZ` bits, which flush **single-precision (float)** denormals to zero but do **not** protect `double`-precision (64-bit) SSE2 operations.

**The problem:** The RLB high-pass biquad (Stage 2, pole magnitude ≈ 0.9946 at 44.1 kHz) decays toward zero during silence. After approximately 3 seconds of silence (130,800 samples), `z1` and `z2` reach double-precision denormal territory (< 2.2×10⁻³⁰⁸). Every arithmetic operation on a denormal double is ~100× slower than normal. On typical audio hardware (sample rate 44.1 kHz, short-burst silence between music), this causes a measurable CPU spike when audio resumes.

This affects any use of the plugin with silence gaps (e.g., between tracks in a mastering session).

## Produces
None

## Consumes
None

## Relevant Files
Modify: `M-LIM/src/dsp/LoudnessMeter.h` — `Biquad::process()` at lines 68–77; add denormal protection
Read: `M-LIM/src/dsp/LoudnessMeter.cpp` — `processBlock()` uses `ScopedNoDenormals` (not sufficient for doubles)

## Acceptance Criteria
- [ ] Run: `cd M-LIM/build && ctest -R LoudnessMeter --output-on-failure` → Expected: all LoudnessMeter tests pass
- [ ] Run: `grep -n "z1\|z2" M-LIM/src/dsp/LoudnessMeter.h` → Expected: at least one denormal-guard expression visible in `process()` or the struct

## Tests
None (performance fix; correctness verified by existing LoudnessMeter test suite)

## Technical Details

Add a tiny DC-injection trick to prevent doubles entering denormal territory. The standard technique is to add and subtract a constant smaller than any real signal but above the denormal threshold:

```cpp
float process(float x) noexcept
{
    constexpr double kDenormalFix = 1e-25;  // above double denormal, below -500 dBFS
    double xd = static_cast<double>(x);
    double y  = b0 * xd + z1;
    z1 = b1 * xd - a1 * y + z2 + kDenormalFix;
    z2 = b2 * xd - a2 * y - kDenormalFix;
    return static_cast<float>(y);
}
```

The paired add/subtract on z1 and z2 cancel each other out across two samples, producing no DC error at audio frequencies while keeping state variables above the denormal threshold. This technique is used in JUCE's own IIR filter implementations and is well-established in professional DSP code.

Alternative: change `z1`/`z2` to `float` (reducing precision), but double precision is important for loudness meter accuracy.

## Dependencies
None
