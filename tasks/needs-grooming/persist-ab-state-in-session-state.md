# Task: Persist ABState Snapshots in getStateInformation / setStateInformation

## Description
`MLIMAudioProcessor::getStateInformation()` only serializes the APVTS state:

```cpp
auto state = apvts.copyState();
std::unique_ptr<juce::XmlElement> xml(state.createXml());
copyXmlToBinary(*xml, destData);
```

`ABState` (the A/B comparison system) stores two `juce::ValueTree` snapshots
(`stateA`, `stateB`) and an `activeIsA` flag. These are NOT included in the
serialized state. When a DAW session is saved and reloaded, the A/B snapshots
are silently discarded and both slots are empty.

For a professional mastering limiter, A/B comparison is a core workflow feature.
Users typically set up A and B variants before saving their session, expecting both
to be preserved on reload. Losing them on every session reload undermines the feature.

**Fix**: Wrap the APVTS state and AB state into a parent XML element for serialization:

```xml
<MLIMState>
  <Parameters>  <!-- existing APVTS state -->
    ...
  </Parameters>
  <ABState activeIsA="1">
    <StateA>
      <Parameters>...</Parameters>
    </StateA>
    <StateB>
      <Parameters>...</Parameters>
    </StateB>
  </ABState>
</MLIMState>
```

`setStateInformation` must remain backward-compatible: if the root element is the
old APVTS tag (not `MLIMState`), load it as the APVTS state directly (no AB state).

## Produces
None

## Consumes
None

## Relevant Files
Modify: `src/PluginProcessor.cpp` — rewrite `getStateInformation()` and
  `setStateInformation()` to include ABState serialization; add backward-compatible
  loading for old-format state
Modify: `src/state/ABState.h` — expose `toXml()` and `fromXml()` (or equivalent)
  methods for serializing the two value-tree snapshots and activeIsA flag
Modify: `src/state/ABState.cpp` — implement serialization helpers
Read: `src/PluginProcessor.h` — member declaration order; `abState` must be
  declared before `apvts` (already true)
Read: `tests/state/test_ab_state.cpp` — existing A/B tests for context

## Acceptance Criteria
- [ ] Run: `cd build && ctest -R "ABState|ab_state|PluginProcessor" --output-on-failure` → Expected: all tests pass
- [ ] Run: `cmake --build build --target MLIM_Standalone_Standalone -j$(nproc) 2>&1 | tail -3` → Expected: build succeeds
- [ ] Test: serialize state with ABState populated, deserialize into a new processor,
  verify both A and B snapshots are restored correctly and activeIsA matches

## Tests
- Unit: `tests/state/test_ab_state.cpp::test_ab_state_round_trips_through_xml` —
  toggle, modify params, toggle back, getStateInformation, setStateInformation into
  fresh processor, verify A and B snapshots are restored and can be toggled correctly
- Unit: `tests/integration/test_plugin_processor.cpp::test_state_backward_compatible_no_ab` —
  call setStateInformation with old-format XML (APVTS root tag only), verify it loads
  without crash and APVTS parameters are correct (ABState defaults to empty snapshots)
- Unit: `tests/state/test_ab_state.cpp::test_active_slot_persisted` — verify that
  `activeIsA` is restored correctly after a round-trip through getState/setState

## Technical Details
Suggested XML structure for ABState serialization:

```cpp
// ABState.h — add:
juce::XmlElement toXml() const;           // serialise stateA, stateB, activeIsA
void fromXml(const juce::XmlElement& e);  // restore snapshots and activeIsA flag
```

In `getStateInformation()`:
```cpp
auto root = std::make_unique<juce::XmlElement>("MLIMState");
root->addChildElement(apvts.copyState().createXml().release());
root->addChildElement(new juce::XmlElement(abState.toXml()));
copyXmlToBinary(*root, destData);
```

In `setStateInformation()`:
```cpp
auto xml = getXmlFromBinary(data, sizeInBytes);
if (xml->hasTagName("MLIMState"))
{
    // New format
    if (auto* apvtsXml = xml->getChildByName(apvts.state.getType()))
        apvts.replaceState(juce::ValueTree::fromXml(*apvtsXml));
    if (auto* abXml = xml->getChildByName("ABState"))
        abState.fromXml(*abXml);
}
else if (xml->hasTagName(apvts.state.getType()))
{
    // Old format (no AB state) — load APVTS only, AB state defaults to empty
    apvts.replaceState(juce::ValueTree::fromXml(*xml));
}
updateLatency();
```

Note: `ABState::stateA` and `stateB` may be empty (default-constructed) `ValueTree`
objects when the user hasn't used A/B yet. Serialization must handle empty trees
gracefully (e.g., skip the child element, or write an empty `<StateA/>` tag and
handle null on read).

## Dependencies
None
