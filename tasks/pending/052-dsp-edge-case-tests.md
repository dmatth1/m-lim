# Task 052: DSP Edge Case Tests Across All Components

## Description
Add edge case tests that are missing from every DSP component task. Current tests only cover happy paths. Audio plugins must handle degenerate inputs gracefully — empty buffers, NaN/Inf samples, denormal floats, extreme sample rates, mono input to stereo processor, and single-sample buffers. These are real-world scenarios that cause crashes and glitches in production plugins.

## Produces
None

## Consumes
DCFilterInterface
DitherInterface
TruePeakDetectorInterface
OversamplerInterface
SidechainFilterInterface
LoudnessMeterInterface
TransientLimiterInterface
LevelingLimiterInterface

## Relevant Files
Create: `M-LIM/tests/dsp/test_dsp_edge_cases.cpp` — cross-component edge case tests
Read: `M-LIM/src/dsp/DCFilter.h` — DCFilter interface
Read: `M-LIM/src/dsp/Dither.h` — Dither interface
Read: `M-LIM/src/dsp/TruePeakDetector.h` — TruePeakDetector interface
Read: `M-LIM/src/dsp/Oversampler.h` — Oversampler interface
Read: `M-LIM/src/dsp/SidechainFilter.h` — SidechainFilter interface
Read: `M-LIM/src/dsp/LoudnessMeter.h` — LoudnessMeter interface
Read: `M-LIM/src/dsp/TransientLimiter.h` — TransientLimiter interface
Read: `M-LIM/src/dsp/LevelingLimiter.h` — LevelingLimiter interface

## Acceptance Criteria
- [ ] Run: `cd M-LIM && cmake --build build --target MLIMTests -j$(nproc) && cd build && ctest -R test_dsp_edge_cases --output-on-failure` → Expected: all tests pass

## Tests
- Unit: `tests/dsp/test_dsp_edge_cases.cpp::test_all_dsp_zero_length_buffer` — call process() with 0 samples on each DSP component, verify no crash
- Unit: `tests/dsp/test_dsp_edge_cases.cpp::test_all_dsp_single_sample_buffer` — process exactly 1 sample through each component
- Unit: `tests/dsp/test_dsp_edge_cases.cpp::test_nan_input_does_not_propagate` — feed NaN into each DSP component, verify output is not NaN (or component handles gracefully)
- Unit: `tests/dsp/test_dsp_edge_cases.cpp::test_inf_input_does_not_propagate` — feed +Inf/-Inf into each DSP component, verify output is finite
- Unit: `tests/dsp/test_dsp_edge_cases.cpp::test_denormal_input_handling` — feed denormal floats (1e-38), verify output does not contain denormals (performance hazard)
- Unit: `tests/dsp/test_dsp_edge_cases.cpp::test_sample_rate_change_reprepare` — call prepare() with 44100, process, then prepare() with 96000, process again — no crash or invalid state
- Unit: `tests/dsp/test_dsp_edge_cases.cpp::test_very_high_sample_rate` — prepare at 192000 Hz, process a block, verify no crash
- Unit: `tests/dsp/test_dsp_edge_cases.cpp::test_silence_passthrough` — all-zero buffer stays all-zero (or near-zero) through each component with default settings

## Technical Details
- Use Catch2 GENERATE() or SECTION() to parameterize across all DSP components
- NaN test: use `std::numeric_limits<float>::quiet_NaN()`, check output with `std::isfinite()`
- Denormal test: use `std::numeric_limits<float>::denorm_min()`, check output is either 0 or normal
- For TransientLimiter and LevelingLimiter, test with both 1 and 2 channels
- Empty buffer test is critical — many ring buffer implementations crash on 0-length input

## Dependencies
Requires tasks 005, 006, 007, 008, 009, 010, 011, 012
