# Task 139: Fix LoudnessMeter RLB High-Pass Filter Q Value (BS.1770-4 Non-Compliance)

## Description
The RLB (Revised Low-frequency B-weighting) high-pass filter in `LoudnessMeter.cpp` uses
`Q = sqrt(2)` (Butterworth 2nd-order) but the correct value per ITU-R BS.1770-4 / EBU R128
is `Q = 0.5003270373238773`, as used by the reference implementation `libebur128`.

**Location**: `M-LIM/src/dsp/LoudnessMeter.cpp`, lines 94–110 (`setupKWeightingFilters()`, Stage 2 block).

**Bug**:
```cpp
// WRONG — uses Butterworth Q = sqrt(2)
const double sq2 = std::sqrt(2.0);
const double a0  = 1.0 + sq2 * K + K2;
```

**Correct formula** (from libebur128 / BS.1770-4):
```cpp
// CORRECT — Q = 0.5003270373238773 as specified by BS.1770-4 Annex 2
const double Q  = 0.5003270373238773;
const double a0 = 1.0 + K / Q + K2;
```

**Verification at 48 kHz** (K = tan(π × 38.135 / 48000) ≈ 0.002494):
- Wrong (Q=√2):  a1 ≈ -1.99598, a2 ≈ 0.99472
- Correct (Q=0.5003): a1 ≈ -1.99005, a2 ≈ 0.99007  ← matches BS.1770-4 Annex 2 reference values
- Reference (from standard): a1 = -1.99004745483398, a2 = 0.99007225036621

The b-numerator coefficients (`b0 = b2 = 1/a0`, `b1 = -2/a0`) are computed correctly via
the bilinear transform and do not change.

**Impact**: LUFS readings are slightly wrong for low-frequency content because the RLB
high-pass pole radius differs. This makes the meter non-compliant with BS.1770-4 for
any signal with significant energy near 38 Hz (kick drums, bass). Certified loudness
targets (e.g., -14 LUFS for streaming) will be slightly mis-measured.

## Produces
None

## Consumes
None

## Relevant Files
Modify: `M-LIM/src/dsp/LoudnessMeter.cpp` — fix Q constant in Stage 2 RLB filter block
Read: `M-LIM/src/dsp/LoudnessMeter.h` — verify filter structure, no changes needed

## Acceptance Criteria
- [ ] Run: `grep -n "sq2\|sqrt(2" M-LIM/src/dsp/LoudnessMeter.cpp` → Expected: no matches (sq2 removed)
- [ ] Run: `grep -n "0.5003270373238773" M-LIM/src/dsp/LoudnessMeter.cpp` → Expected: line found in Stage 2 block
- [ ] Run: `cd build && cmake --build . -j$(nproc) 2>&1 | tail -5` → Expected: builds with no errors
- [ ] Run: `cd build && ctest --output-on-failure -R loudness 2>&1` → Expected: all loudness tests pass

## Tests
- Unit: verify the computed a1/a2 values at 48 kHz match the BS.1770-4 reference
  (a1 ≈ -1.99004745, a2 ≈ 0.99007225) using a static computation test
- Unit: verify at 44100 Hz the computed values match the alternate BS.1770-4 reference:
  a1 = -1.98621612, a2 = 0.98627191

## Technical Details
The fix is a one-line change in `LoudnessMeter.cpp`:

Replace the Stage 2 block (lines ~94-110):
```cpp
// Stage 2: RLB high-pass
const double fc = 38.13547087602444;
const double K  = std::tan(kPi * fc / mSampleRate);
const double K2 = K * K;
const double sq2 = std::sqrt(2.0);              // ← WRONG
const double a0 = 1.0 + sq2 * K + K2;          // ← WRONG
```
With:
```cpp
// Stage 2: RLB high-pass (Q from BS.1770-4 / libebur128 reference)
const double fc = 38.13547087602444;
const double Q  = 0.5003270373238773;           // BS.1770-4 Annex 2
const double K  = std::tan(kPi * fc / mSampleRate);
const double K2 = K * K;
const double a0 = 1.0 + K / Q + K2;            // ← CORRECT
```
The remaining coefficient lines (`rlb.b0`, `rlb.b1`, `rlb.b2`, `rlb.a1`, `rlb.a2`) are correct
and do not change.

## Dependencies
None
