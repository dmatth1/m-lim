# Task: Fix processBlockBypassed Not Maintaining Proper Bypass Behavior

## Description
`PluginProcessor::processBlockBypassed()` delegates directly to `processBlock()`, but does not set the engine's bypass flag. This means when the **host** engages its bypass (e.g., via the DAW's bypass button), the plugin either:
- Applies full limiter processing if the internal bypass param is OFF (incorrect — host bypass should suppress gain reduction)
- Or passes through the delay path at unity gain if the internal bypass param is ON (correct but coincidental)

The correct behavior: when the host calls `processBlockBypassed()`, audio should pass through the lookahead delay buffer at unity gain (to maintain latency for delay compensation) but should NOT apply gain reduction. This matches what the engine's bypass path already does.

## Produces
None

## Consumes
None

## Relevant Files
Modify: `src/PluginProcessor.cpp` — `processBlockBypassed()` at line 102: should temporarily force engine bypass mode
Read: `src/dsp/LimiterEngine.cpp` — `process()` bypass path at line 238: the engine already has correct delay-through bypass logic

## Acceptance Criteria
- [ ] Run: `cd build && ctest --output-on-failure` → Expected: all tests pass
- [ ] Verify: When host calls `processBlockBypassed()`, the engine passes audio through the lookahead delay at unity gain regardless of the plugin's internal bypass parameter state

## Tests
- Unit: `tests/integration/test_plugin_processor.cpp::test_host_bypass_maintains_latency` — verify processBlockBypassed passes audio through delay path without gain reduction
- Unit: `tests/integration/test_plugin_processor.cpp::test_host_bypass_independent_of_param` — verify host bypass works regardless of internal bypass param state

## Technical Details
The fix should either:
1. Temporarily set the engine bypass before delegating to processBlock, then restore it after. But this has thread-safety concerns since bypass is an atomic read in the process loop.
2. Better: call the engine's bypass-specific path directly instead of going through processBlock:
```cpp
void MLIMAudioProcessor::processBlockBypassed(juce::AudioBuffer<float>& buffer, juce::MidiBuffer&)
{
    juce::ScopedNoDenormals noDenormals;
    limiterEngine.processBypass(buffer);  // new method that always runs the delay-through path
    loudnessMeter.processBlock(buffer);
    // push meter data...
}
```
Or simpler: set bypass true, call processBlock, set bypass false. Since processBlock runs synchronously and bypass is an atomic, this is safe within a single audio callback.

## Dependencies
None
