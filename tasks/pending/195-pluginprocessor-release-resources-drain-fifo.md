# Task 195: Drain MeterData FIFO in releaseResources()

## Description
`MLIMAudioProcessor::releaseResources()` is currently empty. The `mProcessorMeterFIFO` (and the engine's internal FIFO) may hold stale `MeterData` snapshots pushed during the previous audio session. When the host calls `releaseResources()` and then restarts audio with a call to `prepareToPlay()`, the UI timer will pop those stale entries on the next `timerCallback()`, applying outdated level/LUFS values to the meters. This can produce a brief flash of wrong meter readings after every transport stop/start cycle or when the host changes the audio format.

Drain the processor FIFO inside `releaseResources()` so that stale entries are discarded before the next audio session begins.

## Produces
None

## Consumes
None

## Relevant Files
Read: `src/PluginProcessor.h` — declares `mProcessorMeterFIFO` and `releaseResources()`
Modify: `src/PluginProcessor.cpp` — implement the drain in `releaseResources()`
Read: `src/dsp/MeterData.h` — `LockFreeFIFO<MeterData>` API (pop returns false when empty)

## Acceptance Criteria
- [ ] Run: `grep -n "releaseResources" src/PluginProcessor.cpp` → Expected: the function body now drains `mProcessorMeterFIFO` (e.g. a `while (mProcessorMeterFIFO.pop(md)) {}` loop with a dummy `MeterData` local)
- [ ] Run: `cmake --build build -j4 2>&1 | tail -5` → Expected: build succeeds with 0 errors

## Tests
None (no testable unit logic — behaviour is observable only at plugin host level)

## Technical Details
The fix is a one-liner drain loop inside `releaseResources()`:
```cpp
void MLIMAudioProcessor::releaseResources()
{
    MeterData md;
    while (mProcessorMeterFIFO.pop(md)) {}
}
```
The engine's own internal FIFO (`limiterEngine.getMeterFIFO()`) does not need draining here because `processBlock()` always drains it before pushing to the processor FIFO — it never accumulates across sessions.

## Dependencies
None
