# Task 078: handleAsyncUpdate Missing loudnessMeter.prepare() and pushAllParametersToEngine()

## Description
`MLIMAudioProcessor::handleAsyncUpdate()` (`PluginProcessor.cpp` lines 175-199) performs a
full oversampling rebuild but only calls `limiterEngine.prepare()`. Compare with the full
`prepareToPlay()` sequence:

```
prepareToPlay():
  1. limiterEngine.setOversamplingFactor(factor)
  2. limiterEngine.prepare(sr, blockSize, numCh)
  3. loudnessMeter.prepare(sr, numCh)          ← MISSING from handleAsyncUpdate
  4. pushAllParametersToEngine()               ← MISSING from handleAsyncUpdate
  5. updateLatency()                           ← present in handleAsyncUpdate ✓
```

**Missing `loudnessMeter.prepare()`**: After an oversampling change at runtime, the
`LoudnessMeter` is not re-prepared. While the loudness meter operates at the original sample
rate (so its internal state is not technically invalid), JUCE's K-weighting filters and the
integration window accumulators are not reset. If the host changes sample rate at the same
time as the oversampling factor, the loudness meter will produce incorrect readings.

**Missing `pushAllParametersToEngine()`**: After `limiterEngine.prepare()`, the engine
re-reads its atomic parameter values internally during `prepare()`. However, the per-component
`setXxx` calls (lookahead, attack, release, channel link, algorithm params, thresholds) are
also called in `prepare()` from the stored atomics, so this is mostly harmless. The subtle
risk: any parameter setter that sets `mParamsDirty = true` but doesn't immediately update
component state (e.g. `setAlgorithmParams`) is deferred to `applyPendingParams()` on the
audio thread. `pushAllParametersToEngine()` forces those atomics to the correct values
pre-flight so the first processBlock after a rebuild starts in a known state.

## Produces
None

## Consumes
None

## Relevant Files
Modify: `src/PluginProcessor.cpp` — add missing calls inside `handleAsyncUpdate()`
Read: `src/PluginProcessor.cpp` — compare `prepareToPlay()` and `handleAsyncUpdate()` bodies

## Acceptance Criteria
- [ ] Run: `grep -A 30 "handleAsyncUpdate" M-LIM/src/PluginProcessor.cpp` → Expected: calls to `loudnessMeter.prepare` and `pushAllParametersToEngine` are present
- [ ] Run: `cd build && ctest --output-on-failure 2>&1 | tail -5` → Expected: all tests pass

## Tests
Integration: `tests/integration/test_plugin_processor.cpp` — add a test that:
1. Prepares the processor
2. Changes the oversampling factor (triggers `handleAsyncUpdate`)
3. Reads `processor.getLoudnessMeter()` state to confirm it does not crash

## Technical Details
In `handleAsyncUpdate()`, after the `limiterEngine.prepare(sr, blockSize, numCh)` call, add:

```cpp
loudnessMeter.prepare (sr, numCh);
pushAllParametersToEngine();
```

The `sr` and `numCh` locals are already computed above those lines.

## Dependencies
None
