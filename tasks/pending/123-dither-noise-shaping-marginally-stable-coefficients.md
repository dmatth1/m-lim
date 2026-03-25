# Task 123: Dither — Second-Order Noise Shaping at 44.1 kHz Uses Marginally Stable Feedback Loop

## Description

`Dither::process()` implements second-order noise shaping (mode 2) via:

```cpp
v -= mCoeff1 * mError1 + mCoeff2 * mError2;
```

At 44.1 kHz, `prepare()` sets `mCoeff1 = 1.0f, mCoeff2 = -1.0f` (lines 26–28 of `Dither.cpp`).

The noise-shaping error feedback loop has the characteristic polynomial:

```
N(z) = z² - c1·z - c2 = z² - 1·z - (-1) = z² - z + 1 = 0
```

Roots: `z = (1 ± j√3) / 2`  →  `|z| = 1.0` (poles **on the unit circle**).

A feedback loop with poles on the unit circle is **marginally stable**. While bounded audio input keeps the quantisation error bounded in practice, the feedback accumulates without decay. Any DC component in the error signal — possible during sustained clipping or with certain input patterns — can cause the feedback to grow and produce audible noise bursts or click artefacts.

The correct stable second-order noise shaping for 44.1 kHz uses coefficients whose feedback polynomial has poles **strictly inside** the unit circle. A minimal fix is to reduce `c2` slightly (e.g., from `−1.0f` to `−0.95f`) to move the poles to `|z| ≈ 0.975`. For higher audio quality, replace with psychoacoustically optimised F-weighted coefficients.

Reference: Lipshitz, Wannamaker & Vanderkooy (1991) recommend c1 ≈ 1.622, c2 ≈ −0.982 for optimally noise-shaped dithering at 44.1 kHz; however these are more aggressive and the conservative stable fix below is sufficient for correctness.

**At 48 kHz** (`mCoeff1 = 0.95f, mCoeff2 = −0.95f`), the characteristic polynomial is `z² − 0.95z + 0.95 = 0` with `|z| ≈ 0.975 < 1` — this is already stable and does not need to change.

**Fix:** Change the 44.1 kHz branch in `Dither::prepare()` from:
```cpp
mCoeff1 = 1.0f;
mCoeff2 = -1.0f;
```
to:
```cpp
mCoeff1 = 1.0f;
mCoeff2 = -0.95f;   // poles at |z| ≈ 0.975, stable with effective 2nd-order HP shaping
```

This keeps the noise shaping strongly high-pass tilted while ensuring the poles are inside the unit circle.

## Produces
None

## Consumes
None

## Relevant Files
Modify: `M-LIM/src/dsp/Dither.cpp` — change 44.1 kHz c2 from -1.0f to -0.95f (line 28)
Read: `M-LIM/src/dsp/Dither.h` — verify coefficient member declarations
Read: `M-LIM/tests/dsp/test_dither.cpp` — existing tests to ensure they still pass

## Acceptance Criteria
- [ ] Run: `cd /workspace/M-LIM/build && ctest -R Dither --output-on-failure` → Expected: all tests pass, exit 0
- [ ] Run: `grep -n "mCoeff2 = -1.0" /workspace/M-LIM/src/dsp/Dither.cpp` → Expected: no output (the -1.0 value removed)
- [ ] Run: `cd /workspace/M-LIM/build && ctest --output-on-failure` → Expected: all tests pass, exit 0

## Tests
- Unit: `tests/dsp/test_dither.cpp::test_noise_shaping_stable_44k` — prepare at 44100 Hz, enable mode 2 (weighted), process 100000 samples of silence; assert all output samples are finite (no NaN/Inf) and the absolute value of mError1 never exceeds 2 LSBs (confirming the feedback does not diverge)
- Unit: `tests/dsp/test_dither.cpp::test_noise_shaping_stable_48k` — same test at 48000 Hz
- Unit: `tests/dsp/test_dither.cpp::test_noise_shaping_stable_96k` — same test at 96000 Hz (coefficients should be zeroed per existing prepare() logic)

## Technical Details

The stability criterion for the second-order error feedback loop with:
```
v[n] = x[n] + dither[n] - c1·e[n-1] - c2·e[n-2]
e[n] = q(v[n]) - v[n]
```
requires the roots of `z² - c1·z - c2 = 0` to have `|z| < 1`.

With `c1 = 1.0, c2 = -0.95`:
- Discriminant: `1.0 - 4·0.95 = -2.8 < 0` (complex roots)
- `|z|² = c2 product = 0.95 → |z| = √0.95 ≈ 0.975 < 1` ✓

The noise shaping frequency response |N(e^jω)| remains strongly high-pass (near-zero at DC, elevated near Nyquist), preserving the psychoacoustic benefit while ensuring stability.

This is a one-line fix in `Dither.cpp`.

## Dependencies
None
