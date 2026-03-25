# Task 093: getTailLengthSeconds Returns 0 — Lookahead Buffer Not Drained at Stop

## Description
`MLIMAudioProcessor::getTailLengthSeconds()` always returns `0.0` (`PluginProcessor.cpp` line 38).

The plugin uses a lookahead delay of up to 5 ms inside `TransientLimiter`. When the host stops
playback, it sends no further audio buffers unless the plugin reports a non-zero tail. Because
`getTailLengthSeconds()` returns 0, the host never sends the extra blocks needed to flush the
lookahead delay, so the **last 0–5 ms of every audio capture is silently dropped**.

This violates the VST3/AU plugin contract: a processor that delays audio must report a tail
long enough for its internal buffers to drain.

## Produces
None

## Consumes
None

## Relevant Files
Modify: `src/PluginProcessor.cpp` — implement `getTailLengthSeconds()` to return the current lookahead
Read: `src/Parameters.h` — `ParamID::lookahead` (0–5 ms range)
Read: `src/dsp/LimiterEngine.h` — `getLatencySamples()` for reference

## Acceptance Criteria
- [ ] Run: `grep -A2 "getTailLengthSeconds" M-LIM/src/PluginProcessor.cpp` → Expected: implementation returns a non-zero value derived from lookahead parameter
- [ ] Run: `cd build && ctest --output-on-failure 2>&1 | tail -5` → Expected: all tests pass

## Tests
Unit: `tests/integration/test_plugin_processor.cpp` — add a test that sets lookahead to 2 ms,
then verifies `processor.getTailLengthSeconds() >= 0.002`.

## Technical Details
`getTailLengthSeconds()` must return the current lookahead in seconds so the host can send
extra silent buffers to drain it:

```cpp
double MLIMAudioProcessor::getTailLengthSeconds() const
{
    if (pLookahead != nullptr)
        return static_cast<double> (pLookahead->load()) * 0.001;
    return 0.0;
}
```

`pLookahead` is a raw parameter pointer initialised in `initParameterPointers()` (which runs in
the constructor, before `getTailLengthSeconds` is ever called), so it is safe to dereference
here.

The oversampler's own latency is already accounted for by `setLatencySamples`; that is separate
from the tail. The tail is only the portion that needs additional _output_ samples after the
host-side input has ended.

## Dependencies
None
