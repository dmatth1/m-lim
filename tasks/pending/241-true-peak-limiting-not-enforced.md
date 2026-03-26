# Task 241: True Peak Limiting Not Enforced — TruePeakDetector Is Measure-Only

## Description

The plugin is marketed as having "true peak limiting" (CLAUDE.md, SPEC.md). The
`TruePeakDetector` class correctly measures inter-sample peaks using the ITU-R BS.1770-4
4x FIR interpolation, but its output is **only used for display** — it is never fed back
to tighten the limiter threshold. Both `TransientLimiter` and `LevelingLimiter` operate
with sample-domain thresholds (`setThreshold(ceiling)`), which means samples passing the
limiter at 0 dBFS (or any configured ceiling) can still produce inter-sample (true) peaks
up to ~+3 dBTP at worst case (two successive Nyquist-frequency samples at ±1.0).

The only protection against true-peak overshoot is incidental: when oversampling is
enabled (e.g., 4x at 176.4 kHz) the limiter operates at a higher sample rate and
naturally catches more inter-sample energy — but this is not guaranteed to be sufficient,
and with oversampling disabled there is no true-peak protection at all.

### Evidence in Code
`LimiterEngine::snapAndPushMeterData()` (LimiterEngine.cpp, lines 312–346):
```cpp
if (mTruePeakEnabled.load())
{
    mTruePeakL.processBlock(buffer.getReadPointer(0), numSamples);
    ...
}
mTruePkL.store(mTruePeakL.getPeak());   // stored for UI display only
```
Nothing uses `mTruePeakL.getPeak()` to adjust the limiter ceiling.

### Required Fix

Add a `TruePeakDetector` instance to the detection path — either:

**Option A (simpler, recommended):** In `stepApplyCeiling()`, after the main hard-clip,
run a lookahead-free per-sample true-peak check using a small (e.g., 4-tap) FIR
oversampler. If any inter-sample peak exceeds the ceiling, attenuate the output block
slightly (dB correction applied uniformly to the block). This is a single-pass post-limiter
correction. The `TruePeakDetector::processSample()` returns the estimated true peak; if it
exceeds the ceiling, apply a gain trim equal to `ceiling / truePeak`.

**Option B (more correct but complex):** Integrate true-peak detection into the limiter
threshold: before `stepRunLimiters()`, run `TruePeakDetector` on the upsampled block and
derive a per-block attenuation that prevents the post-downsample true peaks from
exceeding the ceiling. This requires analysis of the oversampled-to-output gain relationship.

Option A should be implemented. It adds less than one `kFirTaps=12` sample of additional
latency (already accounted for within the lookahead buffer) and guarantees ITU-R
BS.1770-4 true-peak compliance at the output regardless of oversampling setting.

### Location of Change
- `src/dsp/LimiterEngine.cpp` — `stepApplyCeiling()` (lines 264–276) and/or a new step
- `src/dsp/LimiterEngine.h` — add TruePeakDetector members for post-limit detection

## Produces
None

## Consumes
None

## Relevant Files
Modify: `src/dsp/LimiterEngine.cpp` — add true-peak enforcement step after ceiling clip
Modify: `src/dsp/LimiterEngine.h` — member for output-stage TruePeakDetector(s)
Read: `src/dsp/TruePeakDetector.h` — `processSample()`, `getPeak()`, `reset()`
Read: `src/dsp/TruePeakDetector.cpp` — understand FIR delay (12 taps = 12 sample latency)
Read: `tests/dsp/test_true_peak.cpp` — existing tests
Read: `tests/dsp/test_limiter_engine.cpp` — integration tests

## Acceptance Criteria
- [ ] Run: `cd /workspace/M-LIM/build && ctest -R test_true_peak --output-on-failure` → Expected: all tests pass
- [ ] Run: `cd /workspace/M-LIM/build && ctest -R test_limiter_engine --output-on-failure` → Expected: all tests pass
- [ ] Add test: feed a max-amplitude Nyquist-frequency sine (alternating +1, -1, +1, -1 ...) at 0 dBFS ceiling; after processing, the true peak of the output (measured by `TruePeakDetector`) must be ≤ ceiling + 0.5 dB tolerance

## Tests
- Unit: `tests/dsp/test_limiter_engine.cpp` — add `test_true_peak_not_exceeded` that generates worst-case true-peak input (alternating ±1 samples) and verifies that `mTruePeakL.getPeak()` after `process()` is ≤ 1.0 + small tolerance
- Unit: `tests/dsp/test_true_peak.cpp` — existing tests must all still pass

## Technical Details
- `TruePeakDetector::processSample()` introduces 12 samples of FIR group delay (the filter memory). For per-block correction, run the detector on the whole output block and apply a single worst-case gain trim to the block. This is the simplest RT-safe approach.
- Per-block attenuation: `gain = ceiling / max(truePeak, ceiling)` — if truePeak ≤ ceiling, gain = 1 (no change)
- The gain trim must be applied BEFORE dithering so the dither step still operates at the correct level.

## Dependencies
None
