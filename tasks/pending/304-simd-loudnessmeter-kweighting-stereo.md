# Task 304: SIMD Stereo Processing for LoudnessMeter K-weighting Hot Path

## Description
The K-weighting biquad filter in `LoudnessMeter::processBlock()` processes stereo channels
serially inside the per-sample inner loop. For stereo audio, both channels can be computed
simultaneously using 2-wide SSE2/NEON double-precision SIMD, approximately halving the
K-weighting computation cost.

Current code in `LoudnessMeter.cpp:144–158`:
```cpp
for (int i = 0; i < numSmp; ++i)
{
    double samplePower = 0.0;
    for (int ch = 0; ch < numCh; ++ch)
    {
        float s = buffer.getSample(ch, i);
        s = preFilters[ch].process(s);
        s = rlbFilters[ch].process(s);
        samplePower += static_cast<double>(s) * s;
    }
    mBlockPower += samplePower;
}
```

The `Biquad::process()` function in `LoudnessMeter.h:68–76` uses double-precision arithmetic
with direct form II transposed state. For stereo, the two channels' state variables (z1, z2)
are independent and can be computed in parallel using SSE2 (2× double lanes).

The optimization approach:
- Store all channels' z1 and z2 as SSE2-aligned double pairs
- For each sample, load L and R samples into a `__m128d`, broadcast per-channel x values, and
  accumulate the filter state using vectorized multiply-add
- Sum the squared outputs using the SSE2 lane sum

**Constraint**: The Biquad coefficients (b0, b1, b2, a1, a2) are the same for both channels
(see `setupKWeightingFilters()` which sets `f = pre` for all channels). This simplifies SIMD
since coefficient loads are shared.

**Correctness**: The output power accumulation must remain identical to the scalar path.
Verify with the existing `test_loudness_meter_accuracy.cpp` tests.

## Produces
None

## Consumes
None

## Relevant Files
Modify: `src/dsp/LoudnessMeter.h` — add SSE2-aligned state storage in Biquad or a parallel Biquad2 struct
Modify: `src/dsp/LoudnessMeter.cpp` — replace serial channel loop with SIMD biquad computation
Read: `src/dsp/TruePeakDetector.cpp` — reference for JUCE SIMD usage pattern (`juce::dsp::SIMDRegister<float>`)
Read: `tests/dsp/test_loudness_meter_accuracy.cpp` — correctness tests that must still pass

## Acceptance Criteria
- [ ] Run: `cd /workspace/M-LIM/build && ctest -R loudness --output-on-failure` → Expected: all loudness meter tests pass
- [ ] Run: `cd /workspace/M-LIM/build && ctest --output-on-failure` → Expected: all tests pass, exit 0
- [ ] Run: check that `LoudnessMeter::processBlock()` no longer contains a serial `for (int ch ...)` inner loop — the stereo K-weighting must be SIMD-vectorized for the 2-channel case

## Tests
- Unit: `tests/dsp/test_loudness_meter_accuracy.cpp` — all existing LUFS accuracy tests must pass unchanged
- Unit: add a test `test_simd_stereo_power_matches_scalar` in `test_loudness_meter.cpp` that feeds the same signal through a SIMD-path meter and a reference scalar meter and verifies integrated LUFS agrees within 0.1 LU

## Technical Details
- Use `#include <juce_dsp/juce_dsp.h>` and `juce::dsp::SIMDRegister<double>` (or `__m128d` directly) for 2-wide double SIMD
- Fallback: when `numCh != 2`, keep the existing scalar per-channel loop
- The SIMD path can be enabled with `if constexpr (juce::dsp::SIMDRegister<double>::SIMDNumElements >= 2)` — fall through to scalar otherwise
- Biquad state must be stored as channel-interleaved doubles so SSE2 loads are aligned: `double z1[2]; double z2[2];` instead of `double z1; double z2;` in the Biquad struct
- Coefficient arrays are already identical per channel, so coefficient loads can be broadcast

## Dependencies
None
