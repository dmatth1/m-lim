# Task 113: RotaryKnob::paint() Allocates juce::String on Every Repaint — Cache It

## Description
`RotaryKnob::paint()` (`RotaryKnob.cpp` line 129) constructs a `juce::String`
on every call (60 fps):

```cpp
juce::String valueStr = juce::String (slider.getValue(), 1) + " " + suffixText;
```

`juce::String` allocates heap memory.  At 60 fps this is thousands of small
allocations per second that are immediately freed, creating churn in the
allocator and occasional micro-stutter on the message thread.

Fix:
1. Add a `juce::String cachedValueStr_` member to `RotaryKnob`.
2. Override `Slider::Listener::sliderValueChanged()` (or use `onValueChange`)
   to rebuild `cachedValueStr_` only when the slider value actually changes.
3. In `paint()`, replace the inline construction with a read of `cachedValueStr_`.
4. Initialise `cachedValueStr_` in the constructor and whenever `setSuffix()` is
   called, so the suffix change is also reflected immediately.

Note: `suffixText` is a `juce::String` member already; appending to it in paint
is the only allocation site.

## Produces
None

## Consumes
None

## Relevant Files
Modify: `M-LIM/src/ui/RotaryKnob.h` — add cachedValueStr_ member, declare update helper
Modify: `M-LIM/src/ui/RotaryKnob.cpp` — cache in onValueChange/setSuffix, remove inline construction in paint

## Acceptance Criteria
- [ ] Run: `grep -n "juce::String (slider.getValue" M-LIM/src/ui/RotaryKnob.cpp` → Expected: no matches
- [ ] Run: `grep -n "cachedValueStr_" M-LIM/src/ui/RotaryKnob.h` → Expected: member declaration present
- [ ] Run: `cd M-LIM && cmake --build build -j$(nproc) 2>&1 | tail -5` → Expected: build succeeds
- [ ] Run: `cd M-LIM && cd build && ctest --output-on-failure 2>&1 | tail -5` → Expected: all tests pass

## Tests
None — no logic change; display is functionally identical.

## Technical Details
- Use `slider.onValueChange = [this] { updateCachedValue(); };` set in the
  RotaryKnob constructor after the Slider is configured.
- `setSuffix()` should call the same `updateCachedValue()` helper.
- `updateCachedValue()` is a private method that rebuilds `cachedValueStr_` from
  `slider.getValue()` and `suffixText`.

## Dependencies
None
