# Task 010: Loudness Meter (LUFS)

## Description
Implement ITU-R BS.1770-4 compliant loudness metering with momentary, short-term, integrated LUFS measurements and loudness range (LRA).

## Produces
Implements: `LoudnessMeterInterface`

## Consumes
None

## Relevant Files
Create: `M-LIM/src/dsp/LoudnessMeter.h` — class declaration
Create: `M-LIM/src/dsp/LoudnessMeter.cpp` — implementation
Create: `M-LIM/tests/dsp/test_loudness_meter.cpp` — unit tests
Read: `SPEC.md` — LoudnessMeterInterface

## Acceptance Criteria
- [ ] Run: `cd M-LIM && cmake --build build --target MLIMTests -j$(nproc) && cd build && ctest -R test_loudness_meter --output-on-failure` → Expected: all tests pass

## Tests
- Unit: `tests/dsp/test_loudness_meter.cpp::test_silence_returns_negative_infinity` — silence should give -INFINITY LUFS
- Unit: `tests/dsp/test_loudness_meter.cpp::test_1khz_sine_loudness` — 1kHz sine at -20dBFS should measure approximately -20 LUFS (within 1 LU)
- Unit: `tests/dsp/test_loudness_meter.cpp::test_momentary_vs_shortterm` — momentary uses 400ms window, short-term uses 3s window
- Unit: `tests/dsp/test_loudness_meter.cpp::test_integrated_accumulates` — integrated LUFS accumulates over time with gating
- Unit: `tests/dsp/test_loudness_meter.cpp::test_reset_integrated` — resetIntegrated() clears accumulation

## Technical Details
- K-weighting filter chain: pre-filter (high shelf +4dB at ~1500Hz) + RLB weighting (high-pass ~38Hz)
- Both filters are second-order IIR (biquad)
- Momentary: 400ms sliding window, updated every 100ms
- Short-term: 3s sliding window, updated every 100ms
- Integrated: gated measurement per BS.1770-4 (absolute gate at -70 LUFS, relative gate at -10 LU below absolute-gated level)
- LRA: statistical distribution of short-term measurements (10th to 95th percentile range)
- Use ring buffers for windowed measurements
- All measurements in LUFS (Loudness Units relative to Full Scale)

## Dependencies
Requires task 001
