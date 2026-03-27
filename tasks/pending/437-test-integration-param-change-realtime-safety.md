# Task 437: Integration — Real-Time Parameter Change Safety Under Concurrent Load

## Description
The existing `test_realtime_safety.cpp` verifies no heap allocations during `processBlock()`
in steady state, but does not test concurrent parameter changes while the audio thread runs.

Tests needed:
1. **Parameter sweep**: threshold sweeping from -20 to 0 dBFS concurrently with processing — assert no NaN/Inf output
2. **Algorithm switch**: cycling through all 8 algorithms during active limiting — assert no output burst > 1.01f
3. **Bypass toggle**: alternating bypass on/off every block — assert no torn output or NaN
4. **Concurrent param changes + no allocation**: setter-hammering thread + AllocGuard — expect 0 audio-thread allocations

## Produces
None

## Consumes
None

## Relevant Files
Read: `tests/integration/test_realtime_safety.cpp` — existing allocation safety tests
Read: `src/dsp/LimiterEngine.h` — parameter setter thread safety guarantees
Read: `src/PluginProcessor.h` — how parameters flow to LimiterEngine
Modify: `tests/integration/test_realtime_safety.cpp` — add concurrent-change tests

## Acceptance Criteria
- [ ] Run: `cd build && ctest -R "RealtimeSafety" --output-on-failure` → Expected: all tests pass, exit 0

## Tests
- Integration: `tests/integration/test_realtime_safety.cpp::test_threshold_sweep_no_nan`
- Integration: `tests/integration/test_realtime_safety.cpp::test_algorithm_switch_no_overshoot`
- Integration: `tests/integration/test_realtime_safety.cpp::test_bypass_toggle_no_torn_output`
- Integration: `tests/integration/test_realtime_safety.cpp::test_concurrent_param_changes_no_allocation`

## Technical Details
Use `std::atomic<bool> done` to coordinate threads. Algorithm switch tolerance: brief
(~10 sample) transient OK, but no sample must exceed 1.0f ceiling. Use existing
AllocGuard mechanism for the allocation test.

## Dependencies
None
