# Task 005: DC Offset Filter

## Description
Implement the DC offset removal filter — a simple first-order high-pass filter at a very low cutoff frequency (~5 Hz).

## Produces
Implements: `DCFilterInterface`

## Consumes
None

## Relevant Files
Create: `M-LIM/src/dsp/DCFilter.h` — class declaration
Create: `M-LIM/src/dsp/DCFilter.cpp` — implementation
Create: `M-LIM/tests/dsp/test_dc_filter.cpp` — unit tests
Read: `SPEC.md` — DCFilterInterface

## Acceptance Criteria
- [ ] Run: `cd M-LIM && cmake --build build --target MLIMTests -j$(nproc) && cd build && ctest -R test_dc_filter --output-on-failure` → Expected: all tests pass

## Tests
- Unit: `tests/dsp/test_dc_filter.cpp::test_removes_dc_offset` — feed signal with DC offset, verify output has near-zero DC
- Unit: `tests/dsp/test_dc_filter.cpp::test_passes_audio_signal` — verify 1kHz sine passes through with minimal attenuation (<0.1 dB)
- Unit: `tests/dsp/test_dc_filter.cpp::test_reset_clears_state` — verify reset zeros filter state

## Technical Details
- First-order IIR high-pass: y[n] = x[n] - x[n-1] + R * y[n-1], where R = 1 - (2*pi*fc/sr)
- Cutoff frequency: ~5 Hz (effectively removes DC without affecting audio)
- Must handle prepare() with different sample rates
- Process in-place on float array

## Dependencies
Requires task 001
