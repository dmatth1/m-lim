# Task 114: LoudnessMeter LRA with Dynamic Content Test

## Description
`LoudnessMeter::getLoudnessRange()` is tested only with a **constant-loudness signal**,
which verifies that LRA ≈ 0 LU. There is **no test** that a dynamically varying signal
produces a non-zero LRA.

This is a significant gap: if the LRA histogram accumulation or the 10th/95th percentile
computation is broken (e.g., always returning 0), the only existing test (`test_lra_zero_for_constant_signal`)
would still pass because it expects 0.

Add tests to `tests/dsp/test_loudness_meter.cpp` that verify:

1. **Two-loudness signal produces non-zero LRA**: Feed 20s of a quiet tone (-30 dBFS) then
   20s of a loud tone (-10 dBFS). The loudness range should be > 1 LU (and roughly 20 LU
   since the window blocks span both levels).

2. **LRA increases monotonically with increased loudness variation**: Signal A alternates
   between -20 and -30 dBFS (10 LU variation). Signal B alternates between -20 and -40 dBFS
   (20 LU variation). LRA(B) should be greater than LRA(A).

3. **LRA is non-negative**: For any signal, `getLoudnessRange() >= 0.0f`.

4. **Constant signal LRA is near zero** (existing test already covers this — keep it;
   add the above as new tests).

## Produces
None

## Consumes
None

## Relevant Files
Read: `src/dsp/LoudnessMeter.h` — getLoudnessRange interface
Read: `tests/dsp/test_loudness_meter.cpp` — existing LRA test to understand setup pattern
Modify: `tests/dsp/test_loudness_meter.cpp` — add 3 new LRA tests

## Acceptance Criteria
- [ ] Run: `cd /workspace/M-LIM/build && ctest -R "LoudnessMeter" --output-on-failure` → Expected: all tests pass
- [ ] Run: `cd /workspace/M-LIM/build && ctest --output-on-failure 2>&1 | grep -E "passed|failed"` → Expected: no regressions

## Tests
- Unit: `tests/dsp/test_loudness_meter.cpp::test_lra_nonzero_for_dynamic_content` — quiet then loud signal → LRA > 1.0 LU
- Unit: `tests/dsp/test_loudness_meter.cpp::test_lra_increases_with_wider_variation` — wider loudness swing → larger LRA
- Unit: `tests/dsp/test_loudness_meter.cpp::test_lra_always_nonnegative` — multiple signal types → getLoudnessRange() >= 0.0f

## Technical Details
EBU R128 LRA is computed from the 10th and 95th percentile of a distribution of short-term
(3s) loudness blocks (with absolute and relative gating at -70 and -20 LUFS relative gate).

For the two-loudness test: need at least ~40s of audio so the 3s windows span both levels.
The loudness difference (20 LU) should result in an LRA roughly in the range 5–20 LU
(exact depends on gating, but definitely > 1 LU).

```cpp
LoudnessMeter meter;
constexpr double fs = 48000.0;
meter.prepare(fs, 2);

const float loudAmp  = static_cast<float>(std::pow(10.0, -10.0/20.0));  // -10 dBFS
const float quietAmp = static_cast<float>(std::pow(10.0, -30.0/20.0));  // -30 dBFS

// Alternate: 20s loud, 20s quiet, 20s loud — enough to build histogram
feedSine(meter, 1000.0, loudAmp,  fs, 20.0);
feedSine(meter, 1000.0, quietAmp, fs, 20.0);
feedSine(meter, 1000.0, loudAmp,  fs, 20.0);

const float lra = meter.getLoudnessRange();
INFO("LRA for dynamic content: " << lra);
CHECK(lra > 1.0f);
```

## Dependencies
None
