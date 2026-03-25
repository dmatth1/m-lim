# Task 008: Oversampler Wrapper

## Description
Create a wrapper around JUCE's dsp::Oversampling class that provides a clean interface for the limiter engine, supporting factors from off (1x) to 32x.

## Produces
Implements: `OversamplerInterface`

## Consumes
None

## Relevant Files
Create: `M-LIM/src/dsp/Oversampler.h` — class declaration
Create: `M-LIM/src/dsp/Oversampler.cpp` — implementation
Create: `M-LIM/tests/dsp/test_oversampler.cpp` — unit tests
Read: `SPEC.md` — OversamplerInterface

## Acceptance Criteria
- [ ] Run: `cd M-LIM && cmake --build build --target MLIMTests -j$(nproc) && cd build && ctest -R test_oversampler --output-on-failure` → Expected: all tests pass

## Tests
- Unit: `tests/dsp/test_oversampler.cpp::test_2x_upsample_doubles_samples` — verify upsampled block has 2x samples
- Unit: `tests/dsp/test_oversampler.cpp::test_downsample_restores_length` — verify downsampled block matches original length
- Unit: `tests/dsp/test_oversampler.cpp::test_passthrough_when_off` — factor=0 should not alter audio
- Unit: `tests/dsp/test_oversampler.cpp::test_latency_reporting` — verify getLatencySamples returns correct value per factor

## Technical Details
- Wraps `juce::dsp::Oversampling<float>`
- Factor mapping: 0=off(1x), 1=2x, 2=4x, 3=8x, 4=16x, 5=32x
- On setFactor change, must re-create oversampling object (call prepare again)
- getLatencySamples() returns the oversampling object's latency
- upsample returns AudioBlock at higher rate; downsample writes back to original buffer
- Use IIR filtering type for good frequency response

## Dependencies
Requires task 001
