# Task: LimiterEngine::reset() should drain mMeterFIFO

## Description
`LimiterEngine::reset()` clears all DSP state (delay buffers, envelope state, filters) but
does **not** drain `mMeterFIFO`. After a transport reposition or playback-stop event, the
host calls `AudioProcessor::reset()` → `PluginProcessor::reset()` → `LimiterEngine::reset()`.
The FIFO may still contain up to 32 stale `MeterData` entries from before the reset.

The UI timer (60 fps) will drain those stale entries and display:
- Incorrect gain-reduction values (e.g., a GR spike from a loud transient that was already
  processed before the reposition)
- Stale waveform samples causing the scrolling display to appear to jump backward

The fix is to drain `mMeterFIFO` at the end of `LimiterEngine::reset()`:

```cpp
// Clear stale meter snapshots so the UI sees silence after reset
MeterData discard;
while (mMeterFIFO.pop(discard)) {}
```

Additionally, reset the atomic meter state to zero so reads from the getters
(`getGainReduction()`, `getTruePeakL()`, `getTruePeakR()`) also return clean values:

```cpp
mGRdB.store(0.0f);
mTruePkL.store(0.0f);
mTruePkR.store(0.0f);
```

Note: `mGRdB`, `mTruePkL`, `mTruePkR` are already reset in the current `reset()`
implementation (lines 138–140) but `mMeterFIFO` is not drained.

## Produces
None

## Consumes
None

## Relevant Files
Modify: `src/dsp/LimiterEngine.cpp` — add FIFO drain at end of `reset()` body
Read: `src/dsp/LimiterEngine.h` — `mMeterFIFO` declaration (LockFreeFIFO<MeterData>)
Read: `src/dsp/MeterData.h` — LockFreeFIFO::pop() API
Read: `src/PluginProcessor.cpp` — `reset()` override that calls `limiterEngine.reset()`

## Acceptance Criteria
- [ ] Run: `cd build && ctest -R MLIMTests --output-on-failure 2>&1 | tail -10` → Expected: all tests pass
- [ ] Run: `cmake --build build --target MLIM_Standalone_Standalone -j$(nproc) 2>&1 | tail -3` → Expected: build succeeds
- [ ] Run: `grep -A 30 "void LimiterEngine::reset" M-LIM/src/dsp/LimiterEngine.cpp | grep "mMeterFIFO"` → Expected: line containing `mMeterFIFO` (the drain loop) is present in the function body

## Tests
- Unit: `tests/dsp/test_limiter_engine.cpp::test_reset_drains_meter_fifo` — process one block of loud audio (to produce meter data), call `reset()`, then call `getMeterFIFO().pop()` and verify it returns `false` (FIFO is empty after reset)
- Unit: `tests/dsp/test_limiter_engine.cpp::test_reset_clears_meter_atomics` — process one block, call `reset()`, verify `getGainReduction() == 0.0f`, `getTruePeakL() == 0.0f`, `getTruePeakR() == 0.0f`

## Technical Details
`LockFreeFIFO::pop()` is non-blocking and safe to call from any thread. The drain loop
is single-threaded here (reset is always called on the message thread with audio suspended).
No synchronization issues.

## Dependencies
None
