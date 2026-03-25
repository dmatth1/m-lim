# Task 034: Update Latency Reporting When Lookahead Changes

## Description
Total plugin latency = lookahead samples + oversampler latency samples. Task 017 mentions updating `setLatencySamples` when oversampling changes, but NOT when the lookahead parameter changes. Since lookahead ranges from 0-5ms (0-240 samples at 48kHz), changing it significantly affects reported latency. The host needs accurate latency for delay compensation. Ensure `setLatencySamples()` is called whenever either lookahead or oversampling factor changes.

## Produces
None

## Consumes
LimiterEngineInterface
PluginProcessorCore

## Relevant Files
Modify: `M-LIM/src/PluginProcessor.cpp` — add listener or check for lookahead parameter changes, call setLatencySamples
Modify: `M-LIM/src/dsp/LimiterEngine.h` — ensure getLatencySamples() accounts for current lookahead value
Modify: `M-LIM/src/dsp/LimiterEngine.cpp` — update latency calculation when lookahead changes
Read: `M-LIM/src/Parameters.h` — lookahead parameter ID

## Acceptance Criteria
- [ ] Run: `cd M-LIM && grep -A5 "lookahead" src/dsp/LimiterEngine.cpp | grep -i "latency"` → Expected: latency recalculated when lookahead changes
- [ ] Run: `cd M-LIM && cmake --build build --target MLIMTests -j$(nproc) && cd build && ctest -R test_latency --output-on-failure` → Expected: all latency tests pass

## Tests
- Integration: `tests/integration/test_plugin_processor.cpp::test_latency_updates_with_lookahead` — set lookahead to 5ms, verify getLatencyInSamples() reflects 5ms worth of samples; change to 0ms, verify latency decreases

## Technical Details
- In processBlock: after reading lookahead parameter, if value changed, call setLatencySamples(limiterEngine.getLatencySamples())
- LimiterEngine.getLatencySamples() must return: (lookahead_ms * sampleRate / 1000) + oversampler.getLatencySamples()
- Use a cached previous value to detect changes without overhead
- Note: frequent setLatencySamples() calls may cause some hosts to introduce small glitches during latency compensation adjustment — this is expected and matches Pro-L 2 behavior

## Dependencies
Requires tasks 013, 017
