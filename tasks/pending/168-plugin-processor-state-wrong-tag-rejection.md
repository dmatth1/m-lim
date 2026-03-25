# Task 168: Test PluginProcessor::setStateInformation() Rejects Wrong Root Tag

## Description
`PluginProcessor::setStateInformation()` at PluginProcessor.cpp:153-158 has a tag-name guard:

```cpp
if (xmlState->hasTagName(apvts.state.getType()))
    apvts.replaceState(juce::ValueTree::fromXml(*xmlState));
```

If the XML is valid but the root tag does not match the APVTS state type, `replaceState` is
silently skipped.  The existing tests cover `nullptr` data and garbage bytes, but **no test
passes valid XML with a mismatched root element tag** to verify the guard works and the state
is left unchanged.

Tests to add in `test_plugin_processor.cpp`:

1. **test_set_state_wrong_tag_leaves_state_unchanged** — Prepare a processor with a known
   parameter value. Construct a valid JUCE XML blob whose root tag is deliberately wrong (e.g.,
   "WrongTag" instead of the APVTS state type), call `setStateInformation()` with it, then verify
   the parameter value is still the known original value (state was not replaced).

2. **test_set_state_correct_tag_replaces_state** — Complementary positive test: build correct XML
   with the right tag (matching `apvts.state.getType()`), call `setStateInformation()`, verify the
   parameter value changed to the one encoded in the XML.  This ensures the guard passes when
   correct and confirms the tag comparison logic is tested in both branches.

## Produces
None

## Consumes
None

## Relevant Files
Read: `M-LIM/src/PluginProcessor.cpp:153-158` — setStateInformation implementation
Read: `M-LIM/tests/integration/test_plugin_processor.cpp:438-460` — existing null/garbage tests to understand test fixture pattern
Modify: `M-LIM/tests/integration/test_plugin_processor.cpp` — add two new TEST_CASE blocks

## Acceptance Criteria
- [ ] Run: `cd M-LIM/build && ctest -R test_plugin_processor --output-on-failure` → Expected: all tests pass including the two new tag tests
- [ ] Run: `grep -c "wrong.*[Tt]ag\|WrongTag\|hasTagName" M-LIM/tests/integration/test_plugin_processor.cpp` → Expected: output >= 2

## Tests
- Unit: `tests/integration/test_plugin_processor.cpp::test_set_state_wrong_tag_leaves_state_unchanged` — wrong-tag XML does not alter parameter values
- Unit: `tests/integration/test_plugin_processor.cpp::test_set_state_correct_tag_replaces_state` — correct-tag XML does replace parameter values

## Technical Details
- To construct wrong-tag XML: create a `juce::XmlElement("WrongTag")`, add the APVTS child XML
  to it, then call `copyXmlToBinary(*xml, block)` to get the binary blob for `setStateInformation`.
- To get the correct tag name: `processor.getAPVTS().state.getType().toString()` or inspect what
  `getStateInformation` produces and parse the root element name.
- Use the existing `makeProcessor()` helper in the test file to avoid boilerplate.

## Dependencies
None
