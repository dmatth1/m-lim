# Task 114: ControlStrip Has Two Buttons for the Same Parameter — Remove Manual Sync

## Description
`ControlStrip` contains two separate `juce::TextButton` members that both
represent the `truePeakEnabled` APVTS parameter:

- `truePeakButton_` — hidden, has the APVTS `ButtonAttachment` (line 377)
- `truePeakLimitingButton_` — visible in the status bar, manually synced via
  two `onClick` lambdas (lines 176–184)

The manual sync pattern (lines 176–184) is error-prone:
```cpp
truePeakLimitingButton_.onClick = [this]
{
    truePeakButton_.setToggleState (truePeakLimitingButton_.getToggleState(), ...);
};
truePeakButton_.onClick = [this]
{
    truePeakLimitingButton_.setToggleState (truePeakButton_.getToggleState(), ...);
};
```
This creates a circular callback risk and can desync if either lambda is
triggered without the other.  JUCE's `ButtonAttachment` should drive exactly one
button.

Fix:
1. Remove `truePeakButton_` (the hidden proxy button) entirely.
2. Change the `ButtonAttachment` in `createAttachments()` to attach directly to
   `truePeakLimitingButton_`.
3. Delete the two `onClick` sync lambdas.
4. Remove the `addChildComponent(truePeakButton_)` call and the `styleToggleButton(truePeakButton_)` call.
5. Remove the `truePeakButton_` declaration from `ControlStrip.h`.

After this change, `truePeakLimitingButton_` is the single source of truth,
driven by APVTS just like all other toggle buttons.

## Produces
None

## Consumes
None

## Relevant Files
Modify: `M-LIM/src/ui/ControlStrip.h` — remove truePeakButton_ member
Modify: `M-LIM/src/ui/ControlStrip.cpp` — remove proxy button setup, rewire attachment

## Acceptance Criteria
- [ ] Run: `grep -n "truePeakButton_" M-LIM/src/ui/ControlStrip.h` → Expected: no matches
- [ ] Run: `grep -n "truePeakButton_" M-LIM/src/ui/ControlStrip.cpp` → Expected: no matches
- [ ] Run: `grep -n "truePeakLimitingButton_" M-LIM/src/ui/ControlStrip.cpp` → Expected: contains ButtonAttachment line
- [ ] Run: `cd M-LIM && cmake --build build -j$(nproc) 2>&1 | tail -5` → Expected: build succeeds
- [ ] Run: `cd M-LIM && cd build && ctest --output-on-failure 2>&1 | tail -5` → Expected: all tests pass

## Tests
None — UI-only change; no new test logic required.

## Technical Details
- `truePeakLimitingButton_` is already fully styled (lines 164–173) and visible;
  it just needs the attachment to drive its toggle state.
- The APVTS attachment will automatically set the button's initial toggle state
  when constructed, so no manual initialisation is needed.
- Verify that `truePeakLimitingButton_` uses `setClickingTogglesState(true)` —
  it already does (line 165).

## Dependencies
None
