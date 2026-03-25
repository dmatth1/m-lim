# Task 060: Add Null Check in PluginProcessor::getStateInformation()

## Description
In `PluginProcessor.cpp` line 69-70, `state.createXml()` can return `nullptr` if the ValueTree is invalid, but the code dereferences it without checking:
```cpp
auto state = apvts.copyState();
std::unique_ptr<juce::XmlElement> xml (state.createXml());
copyXmlToBinary (*xml, destData);  // crashes if xml is nullptr
```

This is a potential crash in DAW state save operations.

## Produces
None

## Consumes
PluginProcessorCore

## Relevant Files
Modify: `M-LIM/src/PluginProcessor.cpp` — add nullptr check before dereferencing xml

## Acceptance Criteria
- [ ] Run: `grep -A3 "getStateInformation" M-LIM/src/PluginProcessor.cpp | grep "if.*xml\|nullptr"` → Expected: null check present
- [ ] Run: `cd M-LIM && cmake --build build --target MLIM_VST3 -j$(nproc) 2>&1 | tail -1` → Expected: builds successfully

## Tests
None (edge case safety check — ValueTree should always be valid in practice)

## Technical Details
Simple fix:
```cpp
void MLIMAudioProcessor::getStateInformation (juce::MemoryBlock& destData)
{
    auto state = apvts.copyState();
    std::unique_ptr<juce::XmlElement> xml (state.createXml());
    if (xml != nullptr)
        copyXmlToBinary (*xml, destData);
}
```

## Dependencies
None
