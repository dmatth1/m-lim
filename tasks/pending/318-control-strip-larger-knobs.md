# Task 318: Increase Control Strip Knob Size to Match Pro-L 2

## Description
The rotary knobs in the control strip are too small compared to Pro-L 2. Analysis shows:
- Current `kKnobRowH = 56` in ControlStrip.cpp constrains knob diameter to **30px**
  (`knobSize = min(139px col width, 56 - 26px text) = 30px`)
- Pro-L 2 reference knobs appear ~42–46px diameter
- This makes the control strip look visually sparse vs. the reference

Increasing `kKnobRowH` from 56 to 72 gives knob diameter = `min(139, 72-26) = 46px`.
`kControlStripH` in PluginEditor.h must increase to match: `10 + 72 + 4 + 20 = 106 → set to 108`.

Also fix the knob label width: currently the label is drawn with width = `knobSize` (30px), causing
"LOOKAHEAD" → "LOOKA..." and "TRANSIENTS" → "TRANSI..." truncation. Change label rect width to
use the full RotaryKnob component width (`bounds.getWidth()`) instead of `knobSize`.

## Produces
None

## Consumes
None

## Relevant Files
Modify: `src/ui/ControlStrip.cpp` — change `kKnobRowH` from 56 to 72
Modify: `src/PluginEditor.h` — change `kControlStripH` from 92 to 108
Modify: `src/ui/RotaryKnob.cpp` — widen the label rect from `(int)knobSize` to `(int)bounds.getWidth()`

## Acceptance Criteria
- [ ] Run: build the standalone and capture a screenshot → Expected: knobs are visibly larger (~46px) and labels "LOOKAHEAD", "ATTACK", "RELEASE", "TRANSIENTS" are no longer truncated
- [ ] Run: `cd /workspace/M-LIM && cmake --build build -j$(nproc) 2>&1 | grep -c "error:"` → Expected: `0`
- [ ] Run: compare control strip crop at RMSE → Expected: lower than 0.2257 (22.57%)

## Tests
None

## Technical Details
In ControlStrip.cpp, the local constant at line ~6:
```cpp
static constexpr int kKnobRowH = 56;
```
Change to:
```cpp
static constexpr int kKnobRowH = 72;
```

In PluginEditor.h line ~65:
```cpp
static constexpr int kControlStripH = 92;
```
Change to:
```cpp
static constexpr int kControlStripH = 108;
```

In RotaryKnob.cpp, the label drawing rect at paint() (line ~101):
```cpp
juce::Rectangle<int>((int)knobX, (int)bounds.getY(), (int)knobSize, (int)labelH),
```
Change to:
```cpp
juce::Rectangle<int>((int)bounds.getX(), (int)bounds.getY(), (int)bounds.getWidth(), (int)labelH),
```

## Dependencies
None
