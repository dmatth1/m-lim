# Task 009: Sidechain Filter

## Description
Implement the sidechain filter with high-pass, low-pass, and tilt EQ capabilities. This filter shapes the signal that the limiter responds to (detection path).

## Produces
Implements: `SidechainFilterInterface`

## Consumes
None

## Relevant Files
Create: `M-LIM/src/dsp/SidechainFilter.h` — class declaration
Create: `M-LIM/src/dsp/SidechainFilter.cpp` — implementation
Create: `M-LIM/tests/dsp/test_sidechain_filter.cpp` — unit tests
Read: `SPEC.md` — SidechainFilterInterface

## Acceptance Criteria
- [ ] Run: `cd M-LIM && cmake --build build --target MLIMTests -j$(nproc) && cd build && ctest -R test_sidechain_filter --output-on-failure` → Expected: all tests pass

## Tests
- Unit: `tests/dsp/test_sidechain_filter.cpp::test_highpass_attenuates_bass` — 100Hz HP filter attenuates 50Hz signal by >6dB
- Unit: `tests/dsp/test_sidechain_filter.cpp::test_lowpass_attenuates_treble` — 5kHz LP filter attenuates 10kHz signal by >6dB
- Unit: `tests/dsp/test_sidechain_filter.cpp::test_tilt_boosts_highs` — positive tilt boosts high frequencies relative to lows
- Unit: `tests/dsp/test_sidechain_filter.cpp::test_flat_passes_signal` — default settings pass signal unchanged

## Technical Details
- High-pass: second-order Butterworth (juce::dsp::IIR::Coefficients::makeHighPass)
- Low-pass: second-order Butterworth (juce::dsp::IIR::Coefficients::makeLowPass)
- Tilt: first-order shelving filter that tilts frequency response around 1kHz
- HP range: 20-2000 Hz (set to 20 Hz = effectively off)
- LP range: 2000-20000 Hz (set to 20000 Hz = effectively off)
- Tilt range: -6 to +6 dB
- Process in-place on AudioBuffer

## Dependencies
Requires task 001
