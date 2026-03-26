# Task 328: Revert Control Strip Knob Height Regression (Task 318)

## Description
Task 318 increased `kKnobRowH` from 56 to 72 and `kControlStripH` from 92 to 108 to make knobs
larger. This caused measurable RMSE regression:
- Control strip: 22.57% → 23.40% (+0.83 pp)
- Waveform region: 20.00% → 22.08% (+2.08 pp, via proportional layout shift)
- Full image: 21.97% → 22.67% (+0.70 pp) at task 321

The Pro-L 2 reference shows compact knobs in a tight control strip. Pixel measurement of the reference
confirms knobs are smaller than task 318's 46px diameter. Reverting restores the correct proportions.

**Also retain the RotaryKnob label width fix from task 318** (label rect width changed from
`(int)knobSize` to `(int)bounds.getWidth()`) — that fix is correct and should NOT be reverted,
as it prevents label truncation regardless of knob size.

## Produces
None

## Consumes
None

## Relevant Files
Modify: `src/ui/ControlStrip.cpp` — revert `kKnobRowH` from 72 back to 56
Modify: `src/PluginEditor.h` — revert `kControlStripH` from 108 back to 92
Read: `src/ui/RotaryKnob.cpp` — keep existing label width fix (bounds.getWidth()), do NOT revert

## Acceptance Criteria
- [ ] Run: `grep "kKnobRowH" /workspace/M-LIM/src/ui/ControlStrip.cpp` → Expected: `kKnobRowH    = 56`
- [ ] Run: `grep "kControlStripH" /workspace/M-LIM/src/PluginEditor.h` → Expected: `kControlStripH  = 92`
- [ ] Run: `cd /workspace/M-LIM && cmake --build build -j$(nproc) 2>&1 | grep -c "error:"` → Expected: `0`
- [ ] Run: RotaryKnob label drawFittedText should use `bounds.getWidth()` not `knobSize` → Verify in RotaryKnob.cpp paint() label section
- [ ] Run: Screenshot + RMSE compare → Expected: control strip RMSE ≤ 23.0%, waveform ≤ 22.0%

## Tests
None

## Technical Details
In `src/ui/ControlStrip.cpp`, change (around line 6):
```cpp
static constexpr int kKnobRowH    = 72;
```
To:
```cpp
static constexpr int kKnobRowH    = 56;
```

In `src/PluginEditor.h`, change (around line 65):
```cpp
static constexpr int kControlStripH  = 108;
```
To:
```cpp
static constexpr int kControlStripH  = 92;
```

**DO NOT change** `src/ui/RotaryKnob.cpp` — keep the current label width fix:
```cpp
juce::Rectangle<int>((int)bounds.getX(), (int)bounds.getY(), (int)bounds.getWidth(), (int)labelH),
```
This was the correct improvement from task 318 and must be kept.

## Dependencies
None
