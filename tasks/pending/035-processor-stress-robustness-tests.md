# Task 035: Plugin Processor Stress and Robustness Tests

## Description
Task 017 tests basic processor operation (1000 blocks, ceiling, state). Missing: rapid parameter automation, buffer size changes mid-session, prepare/release/prepare cycles, and processing with all features enabled simultaneously. These scenarios are routine in DAW usage and are common sources of crashes.

## Produces
None

## Consumes
PluginProcessorCore

## Relevant Files
Create: `M-LIM/tests/integration/test_processor_stress.cpp` — stress and robustness tests
Read: `M-LIM/src/PluginProcessor.h` — processor interface
Read: `M-LIM/src/Parameters.h` — parameter IDs for automation

## Acceptance Criteria
- [ ] Run: `cd M-LIM && cmake --build build --target MLIMTests -j$(nproc) && cd build && ctest -R test_processor_stress --output-on-failure` → Expected: all tests pass

## Tests
- Integration: `tests/integration/test_processor_stress.cpp::test_rapid_parameter_changes` — change every parameter to random valid values between each processBlock call for 500 blocks, verify no crash and output is finite
- Integration: `tests/integration/test_processor_stress.cpp::test_buffer_size_variation` — process blocks of varying sizes (1, 32, 64, 128, 256, 512, 1024, 4096) in sequence, verify no crash
- Integration: `tests/integration/test_processor_stress.cpp::test_prepare_release_cycle` — call prepareToPlay/releaseResources 10 times, then process, verify correct operation
- Integration: `tests/integration/test_processor_stress.cpp::test_all_features_enabled` — enable truePeak + oversampling(4x) + dcFilter + dither + set loud input, process 100 blocks, verify output within ceiling and all finite
- Integration: `tests/integration/test_processor_stress.cpp::test_all_algorithms_process_cleanly` — iterate through all 8 algorithms, process 50 blocks each, verify output never exceeds ceiling and is finite
- Integration: `tests/integration/test_processor_stress.cpp::test_sample_rate_switch` — prepare at 44100, process, prepare at 96000, process, prepare at 48000, process — verify no crash

## Technical Details
- Use `juce::Random` with fixed seed for reproducible parameter randomization
- For buffer size variation, create new AudioBuffer for each size (JUCE processors must handle varying block sizes)
- For all-features test, set: inputGain=+12dB, ceiling=-0.1dB, truePeak=on, oversampling=4x, dcFilter=on, dither=on, ditherBitDepth=16
- Verify output finiteness with `std::isfinite()` on every output sample
- Each algorithm test: set algorithm, process pink noise at -6dBFS, check output ≤ ceiling + 0.1dB tolerance

## Dependencies
Requires task 017
