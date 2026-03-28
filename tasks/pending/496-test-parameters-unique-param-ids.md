# Task 496: Parameters — Unique Parameter ID Tests

## Description
`Parameters::createParameterLayout()` defines all APVTS parameters with string IDs.
A duplicate parameter ID (two parameters with the same ID string) causes silent collision:
one parameter's value silently overwrites the other, producing hard-to-debug behavior where
an automation lane controls the wrong parameter.

JUCE's APVTS does NOT assert on duplicate IDs at construction time; it silently uses the
last one. There is currently no test that verifies all parameter IDs are unique.

This gap means a well-intentioned refactor or copy-paste error that introduces a duplicate
ID would pass all existing tests while breaking production behavior.

## Produces
None

## Consumes
None

## Relevant Files
Read: `src/Parameters.h` — ParamID constants (string literals for each parameter)
Read: `src/Parameters.cpp` — createParameterLayout() function
Read: `tests/integration/test_parameter_state.cpp` — existing parameter tests
Modify: `tests/integration/test_parameter_state.cpp` — add uniqueness test

## Acceptance Criteria
- [ ] Run: `cd build && ctest -R "ParameterState" --output-on-failure` → Expected: all tests pass, exit 0

## Tests
- Unit: `tests/integration/test_parameter_state.cpp::test_all_parameter_ids_are_unique` — build a list of all parameter IDs from `Parameters::createParameterLayout()`, insert into `std::unordered_set<juce::String>`, verify set.size() == total parameter count (no duplicates)
- Unit: `tests/integration/test_parameter_state.cpp::test_all_parameter_ids_are_non_empty` — verify none of the parameter IDs is an empty string
- Unit: `tests/integration/test_parameter_state.cpp::test_all_param_id_constants_match_layout` — verify each `ParamID::*` constant (from Parameters.h) appears in the actual layout returned by createParameterLayout(); catch stale constants that were removed from the layout

## Technical Details
```cpp
#include "Parameters.h"

TEST_CASE("test_all_parameter_ids_are_unique", "[ParameterState]") {
    auto layout = Parameters::createParameterLayout();
    // Iterate over the ParameterLayout to extract all IDs
    std::unordered_set<std::string> seen;
    int total = 0;
    for (auto& param : layout) {  // layout provides begin()/end() or use getParameters()
        REQUIRE_FALSE(param->getParameterID().isEmpty());
        auto id = param->getParameterID().toStdString();
        INFO("Duplicate param ID: " << id);
        REQUIRE(seen.insert(id).second);  // insert returns false for duplicates
        ++total;
    }
    REQUIRE(total > 0);  // sanity: at least one parameter exists
}
```
Check `Parameters.h` for the exact API (`createParameterLayout()` return type). In JUCE 7,
`juce::AudioProcessorValueTreeState::ParameterLayout` supports range-based for or use
`layout.getParameters()`. If iteration isn't available directly, construct a temporary
APVTS and call `apvts.getParameters()` to enumerate.

## Dependencies
None
