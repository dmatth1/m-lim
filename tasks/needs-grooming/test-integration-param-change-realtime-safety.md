# Task: Integration — Real-Time Parameter Change Safety Under Concurrent Load

## Description
The existing `test_realtime_safety.cpp` verifies no heap allocations during `processBlock()` in steady state, but does not test the scenario where parameters change concurrently while the audio thread is running. This is the most common real-world race condition in plugin development:

1. **Parameter sweep during processing**: A host automation lane can send continuous parameter changes (e.g., threshold sweeping from -20 to 0 dBFS) while the audio thread processes. No test verifies that this combination produces valid (non-NaN, non-Inf) output throughout the sweep.

2. **Algorithm switch mid-stream**: Switching from algorithm 0 to algorithm 7 during active limiting should not produce an output burst > 0 dBFS or a NaN sample. Currently tested only in steady state (no concurrent audio thread).

3. **Oversampling factor change under load**: `setOversamplingFactor()` during processing is the most dangerous parameter change (it changes internal buffer sizes). No test verifies this doesn't corrupt the audio stream.

4. **Bypass toggle correctness**: Toggling bypass on/off rapidly (e.g., every block) should produce either the bypassed or processed signal at each block — never a mix (torn write). No test verifies byte-level correctness of the bypass toggle.

## Produces
None

## Consumes
None

## Relevant Files
Read: `M-LIM/tests/integration/test_realtime_safety.cpp` — existing allocation safety tests
Read: `M-LIM/src/dsp/LimiterEngine.h` — parameter setter thread safety guarantees
Read: `M-LIM/src/PluginProcessor.h` — how parameters flow to LimiterEngine
Modify: `M-LIM/tests/integration/test_realtime_safety.cpp` — add concurrent-change tests

## Acceptance Criteria
- [ ] Run: `cd build && ctest -R "RealtimeSafety" --output-on-failure` → Expected: all tests pass, exit 0

## Tests
- Integration: `tests/integration/test_realtime_safety.cpp::test_threshold_sweep_no_nan` — Spin a thread that calls `engine.setThreshold()` in a tight loop from -20 dB to 0 dB, while the main thread processes 1000 audio blocks. Assert no output sample is NaN or Inf.
- Integration: `tests/integration/test_realtime_safety.cpp::test_algorithm_switch_no_overshoot` — Same pattern: one thread cycles through all 8 algorithms while audio thread processes 1000 blocks of 0.9f amplitude. Assert no output sample exceeds 1.01f (ceiling + 1% tolerance for transient).
- Integration: `tests/integration/test_realtime_safety.cpp::test_bypass_toggle_no_torn_output` — Toggle bypass every block (alternating true/false) for 200 blocks. For bypassed blocks: output RMS should equal input RMS ±0.1 dB. For limited blocks: output RMS ≤ ceiling. No block should produce NaN.
- Integration: `tests/integration/test_realtime_safety.cpp::test_concurrent_param_changes_no_allocation` — Use the existing `AllocGuard` mechanism: while a setter-hammering thread runs, measure allocations in the audio thread for 100 blocks. Expect 0 allocations (parameter changes must not cause audio-thread allocation).

## Technical Details
- Use `std::atomic<bool> done` to coordinate threads — the setter thread loops while done=false, then the main thread sets done=true and joins.
- Test tolerance for algorithm switch: a brief (~10 sample) transient is acceptable as the GR envelope adjusts, but no sample should clip hard above 1.0f.
- This test exercises LimiterEngine's atomic setters under real concurrent load, not just single-threaded calls.

## Dependencies
None
