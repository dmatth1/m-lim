# Task 517: Parameter Normalization Boundary Precision Tests

## Description
`test_parameter_state.cpp` verifies parameter ranges and round-trip serialization but does
not test the precision of normalization at the exact boundary values 0.0 and 1.0 (i.e.,
`convertTo0to1(minValue)` should equal exactly 0.0 and `convertTo0to1(maxValue)` should
equal exactly 1.0 — or at minimum be within machine epsilon of those values).

For float parameters, a mismatch here can cause subtle clipping or wrong values when
a host normalises a parameter to the wire-format value 0 or 1 and sends it back.

Additionally, `getDefaultValue()` (normalized) is never checked against the actual default
(denormalized) for consistency. If a parameter's default drifts outside its normalized range
the APVTS will silently clamp it on state restore, producing unexpected default presets.

Add tests for:

1. **Min/max normalization round-trip**: for every float `AudioParameterFloat` in the layout,
   set it to its minimum, read back `convertTo0to1()`, require `<= 1e-5f` from 0.0f. Then
   set to maximum, require `convertTo0to1() >= 1.0f - 1e-5f`.

2. **Default value within range**: for every parameter, require that
   `param->getDefaultValue()` is in [0.0f, 1.0f] (JUCE normalized range). A value outside
   this silently clamps on state restore, masking preset corruption.

3. **Choice parameter index boundary**: for every `AudioParameterChoice`, set the parameter
   to the maximum index value via APVTS (normalized = 1.0), then read back the int index and
   require it equals `choices.size() - 1`. This verifies the choice count stored in the
   layout actually matches the parameter's internal representation.

4. **Value format string non-empty**: for every parameter, call `getCurrentValueAsText()`
   and require the returned string is not empty. Empty value strings appear in the DAW's
   automation lane as blank, confusing the user.

## Produces
None

## Consumes
None

## Relevant Files
Modify: `M-LIM/tests/integration/test_parameter_state.cpp` — add new TEST_CASE blocks
Read: `M-LIM/src/Parameters.h` — understand all declared parameter IDs and ranges
Read: `M-LIM/src/Parameters.cpp` — understand createParameterLayout()

## Acceptance Criteria
- [ ] Run: `cd build && ctest -R "ParameterState" --output-on-failure` → Expected: all tests pass, exit 0
- [ ] Run: `grep -c "convertTo0to1\|getDefaultValue\|getCurrentValueAsText\|normali" M-LIM/tests/integration/test_parameter_state.cpp -i` → Expected: >= 8

## Tests
- Unit: `tests/integration/test_parameter_state.cpp::test_float_param_min_normalizes_to_zero` — all float params: setMin → convertTo0to1 ≈ 0
- Unit: `tests/integration/test_parameter_state.cpp::test_float_param_max_normalizes_to_one` — all float params: setMax → convertTo0to1 ≈ 1
- Unit: `tests/integration/test_parameter_state.cpp::test_all_defaults_in_normalized_range` — all params: getDefaultValue() ∈ [0,1]
- Unit: `tests/integration/test_parameter_state.cpp::test_choice_param_max_index_correct` — all choice params: normalized=1.0 → index == size-1
- Unit: `tests/integration/test_parameter_state.cpp::test_all_params_value_text_nonempty` — all params: getCurrentValueAsText().isNotEmpty()

## Technical Details
- Iterate all parameters via `apvts.processor.getParameters()` and dynamic_cast to
  `juce::AudioParameterFloat`, `juce::AudioParameterChoice`, etc.
- For setting to min/max use `param->setValueNotifyingHost(0.0f)` and `param->setValueNotifyingHost(1.0f)`.
- Use a tolerance of `1e-5f` for float boundary checks (accounts for linear/skewed range
  rounding in JUCE's NormalisableRange).
- If any parameter fails, the test should print the parameter ID in the failure message:
  `CAPTURE(param->getParameterID()); REQUIRE(...)`.

## Dependencies
None
