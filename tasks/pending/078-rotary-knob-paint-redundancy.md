# Task 078: RotaryKnob::paint() Draws Redundant Knob Graphic

## Description
`RotaryKnob::paint()` draws the full knob face, graduation ticks, value arc, and pointer — but the `juce::Slider` child (added with `addAndMakeVisible`) is rendered ON TOP of `RotaryKnob::paint()` output, redrawing all the same elements via `LookAndFeel::drawRotarySlider()`. The knob graphic drawn in `RotaryKnob::paint()` is entirely covered and wasted. Only the **text label section** (below the slider's bounds) is actually visible from `RotaryKnob::paint()`.

This causes:
1. **CPU waste**: knob drawn twice per frame
2. **Inconsistency risk**: `RotaryKnob::paint()` uses `faceRadius = radius * 0.78f` while `LookAndFeel::drawRotarySlider` uses `faceRadius = radius * 0.85f` — different sizes
3. **Incorrect knob size**: the visible knob (from LookAndFeel) is 85% face ratio, but the intended design (matching task 074) is 78%

## Produces
None

## Consumes
LookAndFeelDefinition

## Relevant Files
Modify: `M-LIM/src/ui/RotaryKnob.cpp` — remove the redundant knob-drawing code from paint()
Modify: `M-LIM/src/ui/LookAndFeel.cpp` — fix faceRadius ratio from 0.85f to 0.78f (consistency fix)

## Acceptance Criteria
- [ ] Run: `cd M-LIM && cmake --build build --target MLIM_VST3 -j$(nproc) 2>&1 | tail -3` → Expected: builds successfully
- [ ] Run: `grep -c "faceRadius\|fillEllipse\|graduation\|trackPath\|arcPath\|pointer" M-LIM/src/ui/RotaryKnob.cpp` → Expected: 0 (all knob drawing removed from RotaryKnob::paint)
- [ ] Run: `grep "0\.85f\|0\.78f" M-LIM/src/ui/LookAndFeel.cpp` → Expected: shows 0.78f (not 0.85f)

## Tests
None (visual refactor)

## Technical Details

**What to remove from `RotaryKnob::paint()`:**
The entire knob-drawing section (lines drawing face, graduation ticks, track arc, value arc, pointer rectangle). Keep ONLY:
1. Text label drawing (`g.drawFittedText(labelText, ...)`)
2. Value text drawing (`g.drawFittedText(valueStr, ...)`)

**What to fix in `LookAndFeel::drawRotarySlider()`:**
Change:
```cpp
const float faceRadius = radius * 0.85f;  // WRONG
```
To:
```cpp
const float faceRadius = radius * 0.78f;  // matches Pro-L 2 proportions
```

**After this change:**
- The slider child renders the knob via LookAndFeel (single render, correct proportions)
- RotaryKnob::paint() only renders the text labels below the slider bounds
- Zero duplicate rendering

**Note:** This is a pure refactoring change — no visual regression if LookAndFeel rendering is correct. The visible knob appearance stays the same (or improves due to consistent face radius).

## Dependencies
Requires task 018 (RotaryKnob component), Requires task 003 (LookAndFeel)
Should be done before or together with task 074 (knob face colour/gradient)
