# Task 006: TPDF Dithering with Noise Shaping

## Description
Implement TPDF (triangular probability density function) dithering with multiple bit depths and noise shaping modes.

## Produces
Implements: `DitherInterface`

## Consumes
None

## Relevant Files
Create: `M-LIM/src/dsp/Dither.h` — class declaration
Create: `M-LIM/src/dsp/Dither.cpp` — implementation
Create: `M-LIM/tests/dsp/test_dither.cpp` — unit tests
Read: `SPEC.md` — DitherInterface

## Acceptance Criteria
- [ ] Run: `cd M-LIM && cmake --build build --target MLIMTests -j$(nproc) && cd build && ctest -R test_dither --output-on-failure` → Expected: all tests pass

## Tests
- Unit: `tests/dsp/test_dither.cpp::test_16bit_quantization` — verify output values are quantized to 16-bit steps
- Unit: `tests/dsp/test_dither.cpp::test_24bit_quantization` — verify 24-bit quantization
- Unit: `tests/dsp/test_dither.cpp::test_dither_adds_noise` — verify dithered output differs from simple truncation
- Unit: `tests/dsp/test_dither.cpp::test_noise_shaping_modes` — verify all 3 modes produce different spectral characteristics

## Technical Details
- TPDF dither: sum of two uniform random values, scaled to 2-bit peak-to-peak at target bit depth
- Quantization: round to nearest step at target bit depth
- Bit depths: 16, 18, 20, 22, 24
- Noise shaping modes:
  - Basic (mode 0): no feedback, just TPDF dither
  - Optimized (mode 1): first-order error feedback
  - Weighted (mode 2): second-order shaped error feedback optimized for 44.1kHz
- Use `juce::Random` for dither noise generation

## Dependencies
Requires task 001
