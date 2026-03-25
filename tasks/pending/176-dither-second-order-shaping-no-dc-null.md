# Task 166: Dither — Second-Order Noise Shaping Coefficients Produce No DC Null

## Description

`Dither::prepare()` sets second-order noise shaping coefficients that result in a noise
shaping polynomial with no zero at DC.  The practical effect is that the "Weighted"
(mode 2) dither mode does not actually push quantisation noise towards Nyquist — it
merely redistributes it slightly, providing far less audible benefit than even first-order
shaping.

### The bug

The shaped noise power spectral density is:

```
|N(e^jω)|²  where  N(z) = 1 - c1·z⁻¹ - c2·z⁻²
```

DC null requires `N(1) = 1 - c1 - c2 = 0`.

**At 44.1 kHz** (current: `c1 = 1.0f, c2 = -0.95f`):

```
N(1) = 1 - 1.0 - (-0.95) = 0.95   ← only -0.45 dB DC attenuation
```

**At 48 kHz** (current: `c1 = 0.95f, c2 = -0.95f`):

```
N(1) = 1 - 0.95 - (-0.95) = 1.0   ← zero DC attenuation (flat noise spectrum)
```

At 48 kHz, second-order mode is indistinguishable from no noise shaping at DC.
First-order (mode 1) gives `N(1) = 0` and is therefore **more effective than the
current second-order mode at both sample rates**.

### Root cause

Task 123 fixed marginal stability (c2 from −1.0 to −0.95) but did not fix the
mis-set c1 value. Correct 2nd-order shaping with a near-DC-null requires `c1 ≈ 2·(1−ε)`
so that `N(1) = ε ≈ 0`, while choosing `c2` to keep poles strictly inside the unit circle.

### Correct coefficients

For a near-DC-null with strictly stable poles (`|z| < 1`):

**44.1 kHz**: `c1 = 1.90f, c2 = -0.91f`
```
N(1) = 1 - 1.90 + 0.91 = 0.01  (-40 dB DC attenuation)
poles: z² - 1.9z + 0.91 = 0  → complex, |z| = √0.91 ≈ 0.954 < 1  ✓
```

**48 kHz**: `c1 = 1.80f, c2 = -0.82f`
```
N(1) = 1 - 1.80 + 0.82 = 0.02  (-34 dB DC attenuation)
poles: z² - 1.8z + 0.82 = 0  → complex, |z| = √0.82 ≈ 0.906 < 1  ✓
```

The high-frequency gain at Nyquist (`N(-1)`) increases from ~9 dB to ~22 dB,
properly pushing noise energy above 10 kHz where the ear is less sensitive.

## Produces
None

## Consumes
None

## Relevant Files
Modify: `M-LIM/src/dsp/Dither.cpp` — update mCoeff1 and mCoeff2 in both the 44.1 kHz
  branch (lines ~29–31) and the 48 kHz branch (lines ~21–23), and update the comments to
  reflect the correct pole analysis and DC-null rationale
Read: `M-LIM/src/dsp/Dither.h` — verify coefficient member types
Read: `M-LIM/tests/dsp/test_dither.cpp` — update/extend existing stability tests to also
  verify the noise shaping spectral shape

## Acceptance Criteria
- [ ] Run: `grep -n "mCoeff1\|mCoeff2" /workspace/M-LIM/src/dsp/Dither.cpp` → Expected: 44.1 kHz branch shows `mCoeff1 = 1.90f` and `mCoeff2 = -0.91f`; 48 kHz branch shows `mCoeff1 = 1.80f` and `mCoeff2 = -0.82f`
- [ ] Run: `cd /workspace/M-LIM/build && ctest -R Dither --output-on-failure` → Expected: all Dither tests pass, exit 0
- [ ] Run: `cd /workspace/M-LIM/build && ctest --output-on-failure` → Expected: all tests pass, exit 0

## Tests
- Unit: `tests/dsp/test_dither.cpp::test_second_order_dc_null_44k` — prepare at 44100 Hz,
  enable mode 2, process 8192 samples of DC (constant 0.5f), compute mean of output,
  compare to input mean; the mean absolute error should be < 0.01 LSB (confirming near-DC
  null shapes away the quantisation error at DC)
- Unit: `tests/dsp/test_dither.cpp::test_second_order_dc_null_48k` — same test at 48000 Hz
- Unit: `tests/dsp/test_dither.cpp::test_noise_shaping_stable_44k` — existing stability
  test must still pass (no divergence over 100 000 samples)
- Unit: `tests/dsp/test_dither.cpp::test_noise_shaping_stable_48k` — existing stability
  test must still pass

## Technical Details

The noise shaping feedback structure in `Dither::process()`:
```cpp
v -= mCoeff1 * mError1 + mCoeff2 * mError2;
```

gives shaped noise spectrum `N(z) = 1 - c1·z⁻¹ - c2·z⁻²`.

For a DC null: `c2 = 1 - c1` (exact).  To keep poles strictly inside the unit circle,
choose `c2` slightly away from the exact DC-null condition:

```
c2 = 1 - c1 + ε   (small ε > 0)
N(1) = ε  (near-null, not exact)
pole magnitude: |z| = √|c2|
```

Recommended values are computed by choosing `|z| ≈ 0.95` (44.1 kHz) and `|z| ≈ 0.9`
(48 kHz) with `c2 = -(|z|²)` and `c1 = 1 + c2`:
- 44.1 kHz: `|z| = √0.91 ≈ 0.954`, `c2 = -0.91`, `c1 = 1 - (-0.91) - 0.01 = 1.90`
- 48 kHz: `|z| = √0.82 ≈ 0.906`, `c2 = -0.82`, `c1 = 1 - (-0.82) - 0.02 = 1.80`

Update the comments in `Dither.cpp` to document the DC null calculation and pole
magnitudes for each sample rate branch.

## Dependencies
None
