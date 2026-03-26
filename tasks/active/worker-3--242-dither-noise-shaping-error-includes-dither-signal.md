# Task 242: Dither Noise Shaping Error Term Incorrectly Includes Dither Signal

## Description

In `Dither::process()` the noise-shaping error is computed as:
```cpp
float v = data[i] + dither;           // dither added BEFORE feedback subtraction
// (feedback subtracted from v in-place)
const float quantized = std::round(v / step) * step;
const float error = quantized - v;    // error = quantized - (signal + dither + feedback)
mError2 = mError1;
mError1 = error;
```

The error fed back into the next sample's pre-quantisation value is:
```
e[n] = Q(x[n] + d[n] + ns_feedback) − (x[n] + d[n] + ns_feedback)
```

This means the noise shaper treats the dither signal `d[n]` as part of the "signal error"
to be corrected, and the feedback loop partially cancels the dither noise. The standard
treatment (Vanderkooy & Lipshitz 1984; AES17) defines the feedback error **without** the
dither component:
```
e[n] = Q(x[n] + ns_feedback + d[n]) − (x[n] + ns_feedback)
```
i.e., dither is added to the pre-quantisation value but is NOT included in the error.

**Consequence:** The current implementation reduces the effective dither amplitude at
lower frequencies (where the shaped noise has gain < 1), partially defeating the TPDF
decorrelation property. The spectral content of the residual quantisation error differs
from the design intent of the chosen noise-shaping coefficients, and the quantisation
noise becomes slightly correlated with the signal at low levels.

### Location
`src/dsp/Dither.cpp`, `process()`, lines 95–115.

### Current (incorrect) pattern:
```cpp
float v = data[i] + dither;
if (mNoiseShaping == 1)  v -= mError1;
else if (mNoiseShaping == 2)  v -= mCoeff1*mError1 + mCoeff2*mError2;

const float quantized = std::round(v / step) * step;
const float error = quantized - v;     // ← v already contains dither
mError2 = mError1;
mError1 = error;
```

### Required Fix:
```cpp
// Apply noise shaping feedback to the (undithered) signal
float v = data[i];
if (mNoiseShaping == 1)  v -= mError1;
else if (mNoiseShaping == 2)  v -= mCoeff1*mError1 + mCoeff2*mError2;

// Add TPDF dither after feedback (dither is not part of the error signal)
const float quantized = std::round((v + dither) / step) * step;

// Error is relative to the undithered+feedback value (not including dither)
const float error = quantized - v - dither;   // OR: quantized - (v + dither) + dither = quantized - v
// Simplification: error = quantized - v (since dither cancels)
const float error = quantized - v;
mError2 = mError1;
mError1 = error;

data[i] = quantized;
```

Wait — let us be precise. The standard error definition is:
```
e[n] = quantized - (v + dither)   where v = signal + feedback
     = quantized - v - dither
```
vs the correct form:
```
e[n] = quantized - v   (v = signal + feedback, dither not in error)
```

The fix is: compute `v` (signal + feedback), then quantise `v + dither`, then store
`error = quantized - v` (not `quantized - v - dither`).

## Produces
None

## Consumes
None

## Relevant Files
Modify: `src/dsp/Dither.cpp` — `process()` lines 95–118
Read: `src/dsp/Dither.h` — class members (`mError1`, `mError2`, `mCoeff1`, `mCoeff2`)
Read: `tests/dsp/test_dither.cpp` — existing tests

## Acceptance Criteria
- [ ] Run: `cd /workspace/M-LIM/build && ctest -R test_dither --output-on-failure` → Expected: all tests pass
- [ ] Add test: with second-order noise shaping (mode=2) at 16-bit and a DC input at 0.5 * LSB, verify that after 1000 samples the mean output is within 0.01 LSB of 0.5 * LSB (TPDF dither must not be systematically cancelled)

## Tests
- Unit: `tests/dsp/test_dither.cpp` — add `test_noise_shaping_dither_not_cancelled`: with mode=2, DC input at 0.5 LSB level, 10000 samples, check that the mean of the output quantisation noise is < 0.5 * step (i.e., the dither is not being corrected away by the feedback loop)

## Technical Details
- Simplified change: move `dither` addition to the quantise step, derive error without dither:
  ```cpp
  float v = data[i];
  if (mNoiseShaping == 1)  v -= mError1;
  else if (mNoiseShaping == 2) v -= mCoeff1*mError1 + mCoeff2*mError2;
  const float quantized = std::round((v + dither) / step) * step;
  const float error = quantized - v;  // correct: e = Q(v+d) - v
  ```
- Mode 0 (no noise shaping) is unaffected since there is no feedback loop; the change only matters for modes 1 and 2.

## Dependencies
None
