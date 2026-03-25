# Task 112: LimiterEngine Deferred Oversampling Change Mechanism Tests

## Description
`LimiterEngine::setOversamplingFactor()` uses a deferred-change mechanism: because changing
the oversampling factor requires heap allocation (via `Oversampler::prepare()`), it cannot
be done on the audio thread. Instead, `applyPendingParams()` sets `mDeferredOversamplingChange`
to `true` and returns early, leaving the caller (`PluginProcessor` via its `AsyncUpdater`) to
call `prepare()` on the message thread.

This mechanism has **no tests** at all. Critical failure modes that are undetected:
- `hasDeferredOversamplingChange()` might never become true after `setOversamplingFactor()`
- The audio thread might continue processing at the old factor without setting the flag
- After `prepare()` is called with the new factor, the flag might not be cleared

Add tests to `tests/dsp/test_limiter_engine.cpp` (or a new file
`tests/integration/test_limiter_engine_oversampling.cpp`) that verify:

1. **Flag is not set when factor unchanged**: After `prepare()` and processing one block
   with the same factor, `hasDeferredOversamplingChange()` returns false.

2. **Flag becomes true after factor change on audio thread**: Call `setOversamplingFactor(2)`,
   then process one block. `hasDeferredOversamplingChange()` must return true.

3. **Flag clears after re-prepare**: After `hasDeferredOversamplingChange()` returns true,
   call `prepare()` with the new factor. After the next block, `hasDeferredOversamplingChange()`
   returns false.

4. **Audio is still bounded during deferred period**: While the change is pending (before
   re-prepare), audio continues to be processed (at old factor) and must remain below ceiling.

5. **Latency changes after re-prepare with new factor**: `getLatencySamples()` should reflect
   the latency for the new oversampling factor after `prepare()` is called.

## Produces
None

## Consumes
None

## Relevant Files
Read: `src/dsp/LimiterEngine.h` — hasDeferredOversamplingChange, setOversamplingFactor, getLatencySamples
Read: `src/dsp/LimiterEngine.cpp` — applyPendingParams(), deferred oversampling logic
Read: `src/PluginProcessor.cpp` — how AsyncUpdater is used to call prepare() after deferred change
Modify: `tests/dsp/test_limiter_engine.cpp` — add deferred oversampling tests

## Acceptance Criteria
- [ ] Run: `cd /workspace/M-LIM/build && ctest -R "LimiterEngine" --output-on-failure` → Expected: all tests pass
- [ ] Run: `cd /workspace/M-LIM/build && ctest --output-on-failure 2>&1 | grep -E "passed|failed"` → Expected: no regressions

## Tests
- Unit: `tests/dsp/test_limiter_engine.cpp::test_no_deferred_flag_when_factor_unchanged` — process one block at same factor → flag false
- Unit: `tests/dsp/test_limiter_engine.cpp::test_deferred_flag_set_after_factor_change` — setOversamplingFactor → process block → hasDeferredOversamplingChange() == true
- Unit: `tests/dsp/test_limiter_engine.cpp::test_deferred_flag_cleared_after_reprepare` — flag set → prepare() → process block → flag false
- Unit: `tests/dsp/test_limiter_engine.cpp::test_audio_bounded_during_deferred_period` — process blocks between setOversamplingFactor and prepare() → output within ceiling
- Unit: `tests/dsp/test_limiter_engine.cpp::test_latency_changes_after_reprepare` — getLatencySamples() differs for factor=0 vs factor=2 after respective prepare() calls

## Technical Details
The audio thread calls `applyPendingParams()` at the start of each `process()` block.
If `mOversamplingFactor` differs from `mCurrentOversamplingFactor`, it sets
`mDeferredOversamplingChange = true` and returns.

Test setup (factor change triggers deferred):
```cpp
LimiterEngine engine;
engine.prepare(44100.0, 512, 2);  // factor=0 (from default mOversamplingFactor)
engine.setOversamplingFactor(2);  // trigger dirty flag

juce::AudioBuffer<float> buf(2, 512);
// fill buf ...
engine.process(buf);  // applyPendingParams() runs, sets deferred flag

REQUIRE(engine.hasDeferredOversamplingChange() == true);

// Simulate host action: re-prepare
engine.prepare(44100.0, 512, 2);  // now current factor = 2
engine.process(buf);

REQUIRE(engine.hasDeferredOversamplingChange() == false);
```

## Dependencies
None
