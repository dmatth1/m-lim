# Task 177: TransientLimiter — Per-Sample Transcendental Functions in Release Path

## Description

`TransientLimiter::process()` calls `gainToDecibels()` (`std::log10`) and
`decibelsToGain()` (`std::pow`) inside the per-sample inner loop for gain-state release
smoothing:

**Release path** (`TransientLimiter.cpp` lines ~285–292):
```cpp
const float gDb      = gainToDecibels(g);
const float targetDb = gainToDecibels(target);
const float smoothedDb = gDb + (targetDb - gDb) * (1.0f - mReleaseCoeff);
g = decibelsToGain(smoothedDb);
```

`TransientLimiter` runs at the **upsampled** sample rate.  At 32x oversampling with stereo
(2 x 1,411,200 = 2.82M samples/sec), this loop executes ~2.82M log10 + pow pairs per
second in the release branch alone.  This is the same issue addressed in LevelingLimiter
by task 166 — TransientLimiter has the same pattern and needs the same fix.

### Fix: move release smoothing to linear domain

For release coefficients typical in a fast limiter (mReleaseCoeff in [0.97, 1.0], i.e.
time constants of 10-500 ms), the difference between linear-domain and dB-domain
per-sample IIR is < 0.5% per step.  Replace the dB-domain interpolation with equivalent
linear-domain smoothing:

```cpp
// After (linear domain — 2 multiplies + 1 add per sample):
g = g * mReleaseCoeff + target * (1.0f - mReleaseCoeff);
```

Note: the instant-attack branch (if (target < g) g = target;) is already O(1) with no
transcendentals and needs no change.

## Produces
None

## Consumes
None

## Relevant Files
Modify: `M-LIM/src/dsp/TransientLimiter.cpp` — replace the dB-domain release smoothing
  block inside process() (per-sample loop, release branch, ~lines 285-292) with
  linear-domain equivalent; update inline comment to match
Read: `M-LIM/src/dsp/DspUtil.h` — gainToDecibels / decibelsToGain definitions
Read: `M-LIM/tests/dsp/test_transient_limiter.cpp` — existing tests must still pass

## Acceptance Criteria
- [ ] Run: `grep -n "gainToDecibels\|decibelsToGain" /workspace/M-LIM/src/dsp/TransientLimiter.cpp` -> Expected: no calls inside the per-sample for (int s = 0; s < numSamples; ++s) loop body
- [ ] Run: `cd /workspace/M-LIM/build && cmake --build . -j$(nproc) 2>&1 | tail -5` -> Expected: compiles without errors
- [ ] Run: `cd /workspace/M-LIM/build && ctest -R TransientLimiter --output-on-failure` -> Expected: all TransientLimiter tests pass, exit 0
- [ ] Run: `cd /workspace/M-LIM/build && ctest --output-on-failure` -> Expected: all tests pass, exit 0

## Tests
- Unit: add test in tests/dsp/test_transient_limiter.cpp::test_release_linear_domain_parity
  — process a 0 dBFS step through two TransientLimiter instances, one with dB-domain
  and one with linear-domain release; verify GR curves differ by < 0.5 dB at all
  sample positions after the peak
- Unit: add test in tests/dsp/test_transient_limiter.cpp::test_release_recovery_speed
  — after a 0 dBFS impulse, confirm gain recovers from < -6 dB to > -0.1 dB within
  3 x releaseMs x sampleRate samples (at releaseShape=0.5)
- Unit: tests/dsp/test_transient_limiter.cpp — all existing tests must still pass

## Technical Details

The dB-domain smoothing computes g_new = g^alpha * target^(1-alpha) (geometric mean).
The linear-domain equivalent computes g_new = g*alpha + target*(1-alpha) (arithmetic mean).
For small per-sample steps (alpha near 1.0), they differ by at most (1-alpha)^2 / 2
per step — negligible for release times > 10 ms at any sample rate.

The gainToDecibels(minGain) call at the end of process() that computes mCurrentGRdB
(line ~345) is outside the per-sample loop and is not hot-path — leave it unchanged.

## Dependencies
None
