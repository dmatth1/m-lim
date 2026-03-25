# Task 036: Dither Noise Shaping — Sample Rate Handling

## Description
Task 006 specifies that the "Weighted" noise shaping mode (mode 2) uses "second-order shaped error feedback optimized for 44.1kHz" but does not address what happens at other sample rates (48kHz, 88.2kHz, 96kHz, 192kHz). Noise shaping coefficients designed for 44.1kHz will produce incorrect spectral shaping at other rates — the psychoacoustic weighting curve shifts, potentially pushing shaped noise into audible frequency ranges.

## Produces
None

## Consumes
DitherInterface

## Relevant Files
Modify: `tasks/pending/006-dither.md` — Add sample rate handling requirement for noise shaping
Modify: `M-LIM/src/dsp/Dither.h` — Add prepare(double sampleRate) method
Modify: `M-LIM/src/dsp/Dither.cpp` — Select noise shaping coefficients based on sample rate

## Acceptance Criteria
- [ ] Run: `grep "prepare\|sampleRate" M-LIM/src/dsp/Dither.h` → Expected: prepare method exists
- [ ] Run: `cd M-LIM && cmake --build build --target MLIMTests -j$(nproc) && cd build && ctest -R test_dither --output-on-failure` → Expected: all tests pass

## Tests
- Unit: `tests/dsp/test_dither.cpp::test_noise_shaping_at_48000` — Weighted mode at 48kHz should shape noise differently than at 44.1kHz
- Unit: `tests/dsp/test_dither.cpp::test_high_sample_rate_fallback` — At ≥88.2kHz, noise shaping can fall back to Basic mode (shaping is inaudible above Nyquist/2)

## Technical Details
The Dither interface in SPEC.md lacks a `prepare(double sampleRate)` method. This needs to be added to both the interface definition and the task.

Recommended approach:
- At 44.1kHz: Use F-weighted noise shaping coefficients: `{1.0f, -1.0f}` for the error filter (second-order)
- At 48kHz: Use slightly adjusted coefficients: `{0.95f, -0.95f}`
- At ≥88.2kHz: Fall back to first-order or no noise shaping (the shaped noise frequencies would be ultrasonic anyway)

The `process()` method signature should also accept or internally know the sample rate. Simplest fix: add `void prepare(double sampleRate)` to the Dither class and store the rate, then select coefficients in prepare().

This also means updating SPEC.md's DitherInterface to include `prepare()`.

## Dependencies
Requires task 006 (Dither must exist to verify)
