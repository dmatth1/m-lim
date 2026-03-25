# Task 147: Real-Time Safety — No Test for processBlock With All Features Simultaneously

## Description
`test_realtime_safety.cpp` verifies zero heap allocations in `processBlock()` for the
default engine configuration (no oversampling, default settings). It also tests
`Oversampler` in isolation with 2x oversampling.

However, no test verifies zero allocations when **all optional features are active simultaneously**:
- TruePeak enabled
- Oversampling at 2x (factor=1)
- DC filter enabled
- Dither enabled

This combination exercises every code path in `LimiterEngine::process()`. Any of these
features could independently be allocation-safe, but interact with each other in ways that
trigger heap operations — for example:

1. When oversampling is active, `Oversampler::upsample()` returns an upsampled
   AudioBlock. If the implementation ever re-allocates internal storage based on block
   size, the combined path would allocate.
2. The sidechain oversampler (`mSidechainOversampler`) runs in parallel — its steady-state
   behaviour under oversampling has not been validated for zero allocations.
3. `mPreLimitBuffer` (delta mode snapshot) and `mSidechainBuffer` are pre-allocated in
   `prepare()` for the un-oversampled block size. If oversampling is active, the working
   buffer sizes differ — an allocation could occur if these are resized lazily.

## Produces
None

## Consumes
None

## Relevant Files
Read: `M-LIM/tests/integration/test_realtime_safety.cpp` — existing alloc-tracking infrastructure
Read: `M-LIM/tests/alloc_tracking.h` — AllocGuard, g_trackAllocs
Read: `M-LIM/src/dsp/LimiterEngine.h` — setTruePeakEnabled, setOversamplingFactor, setDCFilterEnabled, setDitherEnabled
Modify: `M-LIM/tests/integration/test_realtime_safety.cpp` — add all-features test

## Acceptance Criteria
- [ ] Run: `cd /workspace/M-LIM/build && ctest -R RealtimeSafety --output-on-failure` → Expected: all tests pass, exit 0

## Tests
- Unit: `tests/integration/test_realtime_safety.cpp::test_all_features_no_alloc` —
  Create a `MLIMAudioProcessor`, set truePeakEnabled=true, dcFilterEnabled=true,
  ditherEnabled=true, oversamplingFactor=1 (2x), then call `prepareToPlay(44100, 512)`.
  Process 10 warm-up blocks (not measured). Then measure allocations for 100 consecutive
  `processBlock()` calls, each with a -6 dBFS sine input. Each block must produce
  `allocCount == 0`.

## Technical Details
- Use the same `AllocGuard` pattern as the existing `test_processblock_no_heap_allocation`
  test (measure per-block, not aggregate).
- Do NOT include the `setValueNotifyingHost` calls for oversamplingFactor inside the
  measured region — parameter changes may trigger JUCE's ValueTree notifications which
  allocate. Set all parameters before the warm-up phase.
- The oversampling change is deferred via `AsyncUpdater`; calling `prepareToPlay()` after
  setting the parameter forces the reallocation synchronously before the measured blocks.
- If this test reveals an allocation, inspect the `AllocGuard::count()` value and add an
  INFO message to identify the call site.

## Dependencies
None
