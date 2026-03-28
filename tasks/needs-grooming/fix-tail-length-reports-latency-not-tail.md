# Task: Fix getTailLengthSeconds Reporting Latency Instead of True Tail Length

## Description
`PluginProcessor::getTailLengthSeconds()` returns `getLatencySamples() / sampleRate`, which is the plugin's **latency** (lookahead + oversampler delay), not its **tail length**. These are different concepts:

- **Latency**: how many samples the plugin delays the signal (for host delay compensation)
- **Tail length**: how long the plugin continues to produce output after input stops (for offline rendering and bus deactivation)

A limiter with lookahead has latency equal to the lookahead, but its tail length depends on the release time — after a loud peak, gain reduction can continue for up to 1000 ms (the max release time). The host needs to know the tail length to avoid cutting off the release envelope during offline bounce or when deactivating a bus.

Currently the tail length only reflects the lookahead (a few ms), so hosts may cut off audio while the limiter is still releasing.

## Produces
None

## Consumes
None

## Relevant Files
Modify: `src/PluginProcessor.cpp` — `getTailLengthSeconds()` at line 46: should return the maximum possible release time (or a conservative estimate)
Read: `src/Parameters.cpp` — Release parameter range: 10-1000 ms (line 57)
Read: `src/dsp/LimiterAlgorithm.h` — Algorithm release shapes affect actual tail

## Acceptance Criteria
- [ ] Run: `cd build && ctest --output-on-failure` → Expected: all tests pass
- [ ] Verify: `getTailLengthSeconds()` returns at least 1.0 second (covers max release time of 1000 ms)

## Tests
- Unit: `tests/integration/test_plugin_processor.cpp::test_tail_length_covers_release` — verify tail length >= 1.0s

## Technical Details
The simplest correct fix: return a conservative constant that covers the worst-case release tail:
```cpp
double MLIMAudioProcessor::getTailLengthSeconds() const
{
    // The limiter's release envelope can take up to 1000 ms (max release param).
    // Add a safety margin for the leveling limiter's adaptive release.
    return 2.0;
}
```
Or dynamically compute from the current release parameter plus a margin. The key insight is that `getTailLengthSeconds()` should NOT include the latency — the host handles latency compensation separately via `getLatencySamples()`.

## Dependencies
None
