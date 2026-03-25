# Task 083: DSP Real-Time Safety Audit — No Heap Allocation in processBlock

## Description
The CLAUDE.md states: "Audio thread uses only atomics and lock-free FIFOs. No allocations or locks in processBlock." There are currently no tests that verify this constraint. A regression where a DSP component allocates on the audio thread would be invisible until it causes a real-time priority inversion under load.

Tests to add:
- Override `operator new` globally to track allocations during `processBlock()`. Verify zero heap allocations occur per block when the processor is in steady state (after `prepareToPlay` and at least 10 warm-up blocks).
- Verify `LimiterEngine::process()` does not allocate when called on a prepared engine.
- Verify oversampling upsample/downsample does not allocate after initial prepare.

Note: This is a test-only technique — instrument the test binary, not the plugin binary. Use a thread-local allocation counter in the test executable.

## Produces
None

## Consumes
None

## Relevant Files
Read: `src/PluginProcessor.cpp` — processBlock() implementation
Read: `src/dsp/LimiterEngine.cpp` — process() implementation
Read: `src/dsp/Oversampler.cpp` — upsample/downsample implementation
Create: `tests/integration/test_realtime_safety.cpp` — new test file for allocation tracking

## Acceptance Criteria
- [ ] Run: `cd /workspace/M-LIM/build && ctest -R RealtimeSafety --output-on-failure` → Expected: all tests pass, exit 0
- [ ] File exists: `tests/integration/test_realtime_safety.cpp`

## Tests
- Integration: `tests/integration/test_realtime_safety.cpp::test_processblock_no_heap_allocation` — after 10 warm-up blocks, count heap allocations during `processBlock()` for 100 blocks; assert count == 0
- Integration: `tests/integration/test_realtime_safety.cpp::test_limiterengine_process_no_alloc` — direct call to `LimiterEngine::process()` in steady state; assert 0 allocations
- Integration: `tests/integration/test_realtime_safety.cpp::test_oversampling_steady_state_no_alloc` — after `prepare()` + 5 warm-up upsample/downsample cycles, assert 0 allocations per cycle

## Technical Details
- Implement allocation counting with a thread-local counter and a global `operator new` override that increments it:
  ```cpp
  static thread_local int g_allocCount = 0;
  void* operator new(size_t sz) { ++g_allocCount; return std::malloc(sz); }
  ```
  Reset `g_allocCount = 0` before the measured region and assert `g_allocCount == 0` after.
- This only works in test builds where you control the entire binary. The override must be in the test file, not production code.
- Be aware that JUCE's `juce::String` construction can allocate — avoid creating JUCE strings inside the measured region of the test setup.
- Add the new file to `tests/integration/CMakeLists.txt` (or equivalent) so it gets compiled and linked.

## Dependencies
None
