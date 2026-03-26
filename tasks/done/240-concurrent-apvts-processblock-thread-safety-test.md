# Task 240: Concurrent APVTS Parameter Writes + processBlock Thread Safety Test

## Description
The realtime safety tests (`test_realtime_safety.cpp`) verify zero heap allocations during `processBlock()`, but there is no test that exercises the actual DAW usage pattern: **one thread calling `processBlock()` continuously while another thread concurrently changes parameters via APVTS**.

This is the most critical concurrent access scenario in a DAW plugin. The CLAUDE.md architecture says "Audio thread uses only atomics and lock-free FIFOs" and "All parameters via APVTS" — but there is no test that runs both simultaneously and verifies:
- No crash or undefined behavior
- Output remains finite (no NaN/Inf produced)
- No data races trigger sanitizers

Add an integration test that spawns an audio-thread simulator and a UI-thread simulator running concurrently for several hundred blocks, with the UI thread randomly changing all parameters while the audio thread processes.

## Produces
None

## Consumes
None

## Relevant Files
Read: `tests/integration/test_realtime_safety.cpp` — existing pattern (zero-alloc tracking, helper setup)
Read: `tests/integration/test_processor_stress.cpp` — stress test patterns and helpers
Read: `src/Parameters.h` — all parameter IDs to change
Read: `src/PluginProcessor.h` — MLIMAudioProcessor interface
Modify: `tests/integration/test_processor_stress.cpp` — add the new concurrent test case

## Acceptance Criteria
- [ ] Run: `cd /workspace/M-LIM/build && ./tests/MLIMTests "[ProcessorStress]" --reporter compact` → Expected: all tests pass with no failures, no crashes, no sanitizer errors

## Tests
- Integration: `tests/integration/test_processor_stress.cpp::test_concurrent_param_changes_during_processing` — audio thread runs 500 blocks of `processBlock()` while UI thread continuously randomizes all float/bool/choice parameters via `apvts.getParameter(id)->setValueNotifyingHost()`. After both threads join: verify output is finite and no exceptions were thrown.

## Technical Details
- Use `std::thread` for both threads with `std::atomic<bool> keepRunning{true}` as stop flag
- Audio thread: prepareToPlay once, then loop 500 blocks of `processBlock()`, set `keepRunning = false` when done
- UI thread: while `keepRunning`, iterate over all parameter IDs from `ParamID::` and call `setValueNotifyingHost()` with random normalized values (use `juce::Random`)
- Verify after join: all samples in last processed buffer are `std::isfinite()`
- The test should not assert that output values are specific — just that no crash/NaN/Inf occurs
- Parameter IDs to cover: at minimum `inputGain`, `threshold`, `ceiling`, `lookahead`, `attack`, `release`, `channelLink`, `algorithm`, `oversamplingFactor`, `truePeakEnabled`, `dcFilterEnabled`, `ditherEnabled`

## Dependencies
None
