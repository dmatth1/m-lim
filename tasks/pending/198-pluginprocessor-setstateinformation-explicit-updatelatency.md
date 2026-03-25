# Task 198: Call updateLatency() explicitly in setStateInformation() after state restore

## Description
`MLIMAudioProcessor::setStateInformation()` restores plugin state by calling `apvts.replaceState()`. Latency is currently updated only indirectly: the APVTS fires `parameterChanged()` for parameters whose values changed, and the registered listener for `ParamID::lookahead` calls `updateLatency()`. This works in the common case, but fails silently in two edge cases:

1. **Lookahead unchanged**: If the restored state has the same lookahead value as the current one, `parameterChanged` is not fired for lookahead, so `updateLatency()` is never called. If the host had previously changed the sample rate and called `prepareToPlay()` (which DOES call `updateLatency()`), this is benign. But if the host calls `setStateInformation()` AFTER `prepareToPlay()` and the lookahead happens to match, the latency report will be stale.

2. **Oversampling-induced latency change**: `parameterChanged` for `oversamplingFactor` triggers `handleAsyncUpdate()`, which correctly calls `updateLatency()` after a `suspendProcessing` cycle. But if both lookahead and oversampling factor are unchanged in the new state, no latency update fires.

Adding an explicit `updateLatency()` call in `setStateInformation()` — after `apvts.replaceState()` — removes these implicit dependencies and makes latency always correct after state restoration, regardless of which parameters changed.

## Produces
None

## Consumes
None

## Relevant Files
Modify: `src/PluginProcessor.cpp` — add `updateLatency()` call in `setStateInformation()` after `apvts.replaceState()`

## Acceptance Criteria
- [ ] Run: `grep -A8 "setStateInformation" src/PluginProcessor.cpp | grep "updateLatency"` → Expected: `updateLatency()` appears in the body of `setStateInformation()`
- [ ] Run: `cmake --build build -j4 2>&1 | tail -5` → Expected: build succeeds with 0 errors
- [ ] Run: `cd build && ctest -R parameter_state --output-on-failure` → Expected: parameter state tests pass

## Tests
None (no existing test covers this edge case — the fix is purely defensive)

## Technical Details
Change in `setStateInformation()`:

```cpp
void MLIMAudioProcessor::setStateInformation (const void* data, int sizeInBytes)
{
    std::unique_ptr<juce::XmlElement> xmlState (getXmlFromBinary (data, sizeInBytes));
    if (xmlState != nullptr)
        if (xmlState->hasTagName (apvts.state.getType()))
        {
            apvts.replaceState (juce::ValueTree::fromXml (*xmlState));
            updateLatency();   // ← add this line
        }
}
```

`updateLatency()` calls `setLatencySamples(limiterEngine.getLatencySamples())`. This reads `mLookaheadMs` (atomic) and `mOversampler.getLatencySamples()` (reads `mOversampling->getLatencyInSamples()` on the current oversampler). The call is cheap and idempotent. It should be placed AFTER `apvts.replaceState()` so the lookahead atomic has already been updated by the APVTS internals before the latency is computed.

Note: `setStateInformation()` is called on the message thread, same as `updateLatency()` — no thread-safety issue.

## Dependencies
None
