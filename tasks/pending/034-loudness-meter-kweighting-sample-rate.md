# Task 034: Loudness Meter — Sample-Rate-Dependent K-Weighting Coefficients

## Description
Task 010 describes the K-weighting filter chain but does not specify that the ITU-R BS.1770-4 filter coefficients are defined for 48kHz and must be recalculated for other sample rates. Audio plugins commonly run at 44.1kHz, 88.2kHz, 96kHz, or 192kHz. Using the 48kHz coefficients at other sample rates will produce incorrect LUFS readings, violating the ±0.1 LU accuracy requirement.

The K-weighting chain consists of two biquad filters:
1. **Pre-filter (Stage 1)**: High shelf, ~+3.9999 dB boost above ~1500 Hz
2. **RLB weighting (Stage 2)**: High-pass at ~38.1 Hz

Both must be computed from their analog prototype using the bilinear transform at the actual sample rate, not hardcoded for 48kHz.

## Produces
None

## Consumes
LoudnessMeterInterface

## Relevant Files
Modify: `tasks/pending/010-loudness-meter.md` — Add coefficient calculation formulas and multi-sample-rate requirement
Modify: `M-LIM/src/dsp/LoudnessMeter.cpp` — Verify coefficients are recalculated in prepare() based on sample rate

## Acceptance Criteria
- [ ] Run: `grep -c "sampleRate" M-LIM/src/dsp/LoudnessMeter.cpp` → Expected: at least 3 (coefficients depend on sample rate)
- [ ] Run: `cd M-LIM && cmake --build build --target MLIMTests -j$(nproc) && cd build && ctest -R test_loudness_meter --output-on-failure` → Expected: all tests pass

## Tests
- Unit: `tests/dsp/test_loudness_meter.cpp::test_1khz_loudness_at_44100` — 1kHz sine at -20dBFS, SR=44100: LUFS ≈ -20 (within 0.5 LU)
- Unit: `tests/dsp/test_loudness_meter.cpp::test_1khz_loudness_at_96000` — same signal at SR=96000: LUFS should match ±0.5 LU of the 44100 result

## Technical Details
The ITU-R BS.1770-4 defines the analog prototype filters. The reference coefficients for 48kHz are:

**Stage 1 (Pre-filter / Head-related transfer function):**
```
b0 =  1.53512485958697
b1 = -2.69169618940638
b2 =  1.19839281085285
a1 = -1.69065929318241
a2 =  0.73248077421585
```

**Stage 2 (RLB / Revised Low-frequency B-curve):**
```
b0 =  1.0
b1 = -2.0
b2 =  1.0
a1 = -1.99004745483398
a2 =  0.99007225036621
```

These are ONLY valid at 48kHz. For other sample rates, the worker must:
1. Convert the 48kHz digital coefficients back to analog domain (inverse bilinear transform), OR
2. Use the known analog prototype parameters and apply bilinear transform at the target sample rate

The simpler approach: use the analog prototype parameters directly.

**Pre-filter analog prototype:**
- Type: High shelf
- Gain: +3.9999 dB
- Center frequency: 1681.974450955533 Hz
- Q: 0.7071752369554196

**RLB analog prototype:**
- Type: High-pass, 2nd order
- Cutoff: 38.13547087602444 Hz
- Q: 0.5003270373238773

Use standard cookbook biquad formulas (Robert Bristow-Johnson) to compute coefficients at any sample rate in `prepare()`.

The `prepare(double sampleRate, int numChannels)` method must recompute both biquad coefficient sets whenever the sample rate changes.

## Dependencies
Requires task 010 (LoudnessMeter must exist to verify)
