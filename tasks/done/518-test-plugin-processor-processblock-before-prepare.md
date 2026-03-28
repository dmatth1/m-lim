# Task 518: PluginProcessor — processBlock Before prepareToPlay Robustness Tests

## Description
Every test in `test_plugin_processor.cpp` calls `prepareToPlay()` before `processBlock()`.
However, a misbehaving DAW host (or a host under certain race conditions) may call
`processBlock()` before `prepareToPlay()`, or call `processBlock()` again after
`releaseResources()` without a subsequent `prepareToPlay()`.

The constructor initialises internal buffers to small default sizes, but if DSP components
rely on buffers allocated in `prepareToPlay()`, a premature `processBlock()` call could:
- Dereference null/unallocated pointers (crash)
- Access out-of-bounds memory in pre-allocated but empty JUCEbuffers
- Produce undefined output silently

This is a real risk during host plugin scanning and live graph rewiring.

Add the following tests:

1. **processBlock before prepareToPlay**: construct `MLIMAudioProcessor`, immediately call
   `processBlock()` with a 512-sample stereo buffer without calling `prepareToPlay()` first.
   Require: no crash (`REQUIRE_NOTHROW`), and output samples are finite or exactly zero
   (a legitimate "safe no-op" result).

2. **processBlock after releaseResources**: call `prepareToPlay()` then `releaseResources()`
   then `processBlock()`. Require: no crash.

3. **processBlock with block size exceeding prepared size**: call `prepareToPlay(44100, 512)`
   then `processBlock()` with a 1024-sample buffer. Require: no crash (JUCE normally
   guarantees `numSamples <= maxBlockSize` but verifying robustness is worthwhile; if the
   implementation asserts/crashes, document it as a known hard requirement and skip).

4. **Rapid prepareToPlay ↔ processBlock interleaving on two threads**: one thread calls
   `prepareToPlay(44100, 512)` in a tight loop (10 iterations), another concurrently calls
   `processBlock()` (10 blocks). Require: no crash and no sanitiser report (run with
   `-fsanitize=address,undefined` via ASAN_OPTIONS=detect_stack_use_after_return=1 if
   available; otherwise just require no crash).

## Produces
None

## Consumes
None

## Relevant Files
Modify: `M-LIM/tests/integration/test_plugin_processor.cpp` — add new TEST_CASE blocks at the end
Read: `M-LIM/src/PluginProcessor.h` — understand prepareToPlay/releaseResources/processBlock lifecycle
Read: `M-LIM/src/PluginProcessor.cpp` — check what prepareToPlay allocates that processBlock requires

## Acceptance Criteria
- [ ] Run: `cd build && ctest -R "PluginProcessor" --output-on-failure` → Expected: all tests pass, exit 0
- [ ] Run: `grep -c "before.*prepare\|releaseResources\|exceed.*prepared\|interleav" M-LIM/tests/integration/test_plugin_processor.cpp -i` → Expected: >= 3

## Tests
- Unit: `tests/integration/test_plugin_processor.cpp::test_processblock_before_preparetoplay_no_crash` — no crash without prepare
- Unit: `tests/integration/test_plugin_processor.cpp::test_processblock_after_release_resources_no_crash` — no crash after release
- Unit: `tests/integration/test_plugin_processor.cpp::test_processblock_oversized_buffer_no_crash` — block > maxBlockSize (if implementation asserts, test can be marked [!shouldfail])
- Integration: `tests/integration/test_plugin_processor.cpp::test_concurrent_prepare_and_processblock_no_crash` — race condition between prepare and process

## Technical Details
- The test for "before prepareToPlay" must construct a fresh `MLIMAudioProcessor` via default constructor without any subsequent setup calls.
- Use `juce::AudioBuffer<float> buf(2, 512); juce::MidiBuffer midi;` as the test buffer.
- For the concurrent test, use `std::thread` with `std::atomic<bool>` barrier to synchronize start. Keep durations short (< 100 ms total).
- If the implementation hard-asserts in debug mode on processBlock-before-prepare (via `jassert`), add a note in the test comment that this is expected debug behaviour and skip the crash test in release mode only.

## Dependencies
None
