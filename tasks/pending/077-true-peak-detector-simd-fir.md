# Task 077: TruePeakDetector — SIMD Vectorize 4-Phase FIR Hot Path

## Description

`TruePeakDetector::processSample()` (called for every sample of every channel during true peak metering) computes 4 simultaneous 12-tap FIR filters in a scalar nested loop:

```cpp
for (int phase = 0; phase < kPhases; ++phase)       // 4 phases
{
    float sum = 0.0f;
    for (int tap = 0; tap < kFirTaps; ++tap)         // 12 taps
    {
        int idx = (mWritePos - 1 - tap + kFirTaps) % kFirTaps;
        sum += kCoeffs[phase][tap] * mBuffer[idx];
    }
    ...
}
```

This is **48 multiply-accumulate (MAC) operations per input sample**, plus 12 modulo operations for buffer addressing. At 48 kHz with true peak enabled on 2 channels, that is:

`48 MAC + 12 mod  ×  48000 samples/sec  ×  2 channels = 5.76M MAC + 1.15M mod/sec`

**SIMD opportunity:** All 4 FIR phase sums can be computed simultaneously. If coefficients are transposed to `kCoeffs[tap][phase]` layout, a single SIMD load of 4 phase coefficients for tap `k` followed by a broadcast of `mBuffer[idx]` computes all 4 phase contributions in one instruction.

**Fix:** Rearrange the coefficient table to `float kCoeffsByTap[kFirTaps][kPhases]` (tap-major order) and use `juce::dsp::SIMDRegister<float>` to accumulate all 4 phase outputs in parallel. Fall back to the scalar path on platforms without SIMD.

The circular buffer addressing can also be simplified: un-roll the circular access by maintaining a linear staging array (`float mLinearBuf[kFirTaps * 2]`) that mirrors the circular buffer. On each new sample, append to both halves of the linear buffer. The FIR convolution then reads a contiguous sub-array — no modulo needed in the inner loop.

## Produces
None

## Consumes
None

## Relevant Files
Modify: `M-LIM/src/dsp/TruePeakDetector.h` — change coefficient layout; add linear staging buffer
Modify: `M-LIM/src/dsp/TruePeakDetector.cpp` — implement SIMD FIR path with fallback
Read: `M-LIM/src/dsp/TruePeakDetector.h` — current coefficient and buffer layout
Read: `M-LIM/src/dsp/LimiterEngine.cpp` — confirms TruePeakDetector is in the hot path (processBlock called every audio block for both channels, lines 315–319)

## Acceptance Criteria
- [ ] Run: `cd /workspace/build && ctest --output-on-failure` → Expected: all tests pass
- [ ] Run: `cd /workspace && grep -n "kCoeffsByTap\|SIMDRegister\|simd" M-LIM/src/dsp/TruePeakDetector.cpp` → Expected: at least one SIMD reference in implementation
- [ ] Run: Verify by code review that the SIMD and scalar paths produce identical peak values (within float epsilon) for a 1 kHz sine wave test signal

## Tests
- Unit: `tests/dsp/test_true_peak_detector.cpp::test_simd_matches_scalar` — feed 1024 samples of a 0 dBFS 1 kHz sine wave through both SIMD and scalar implementations; assert all peak values differ by less than 1e-5f
- Unit: `tests/dsp/test_true_peak_detector.cpp::test_fir_coefficient_integrity` — verify the transposed `kCoeffsByTap` table contains identical values to the original `kCoeffs` table (all 48 coefficients match)

## Technical Details

**Transposed coefficient table** (tap-major for SIMD load):

```cpp
// In TruePeakDetector.h:
// [tap][phase] layout: for each tap, load all 4 phase coefficients at once
static const float kCoeffsByTap[kFirTaps][kPhases];  // defined in .cpp

// Linear staging buffer (avoids modulo in FIR loop):
std::array<float, kFirTaps * 2> mLinearBuf{};
int mLinearPos = 0;
```

**SIMD FIR computation:**

```cpp
// In processSample():
// Update linear staging buffer (write sample at both mLinearPos and mLinearPos + kFirTaps)
mLinearBuf[mLinearPos] = sample;
mLinearBuf[mLinearPos + kFirTaps] = sample;
mLinearPos = (mLinearPos + 1) % kFirTaps;

const float* src = &mLinearBuf[mLinearPos];  // points to oldest sample (no modulo in loop)

juce::dsp::SIMDRegister<float> acc = juce::dsp::SIMDRegister<float>::zeros();
for (int tap = 0; tap < kFirTaps; ++tap)
{
    auto coeffs = juce::dsp::SIMDRegister<float>::fromRawArray(kCoeffsByTap[tap]);
    auto s      = juce::dsp::SIMDRegister<float>::expand(src[tap]);  // broadcast
    acc          = acc + coeffs * s;
}

// acc[0..3] = phase 0..3 outputs
float phaseOutputs[kPhases];
acc.copyToRawArray(phaseOutputs);
float maxAbs = 0.0f;
for (int p = 0; p < kPhases; ++p)
    maxAbs = std::max(maxAbs, std::abs(phaseOutputs[p]));
```

If `juce::dsp::SIMDRegister<float>` is not available (non-SSE target), the implementation must fall back to the original scalar 4-phase loop. The `kCoeffsByTap` table is populated from the original `kCoeffs[phase][tap]` values with indices transposed.

## Dependencies
None
