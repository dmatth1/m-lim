# Task 321: Control Strip — Increase Knob Row Height for Larger Knobs

## Description
The control strip knobs are currently only ~30px in diameter due to the limited knob row
height (`kKnobRowH = 56`, minus 26px for label+value text = 30px usable for the knob face).

The Pro-L 2 reference shows knobs approximately 42–48px in diameter that are visually
prominent and easy to interact with. Our 30px knobs are noticeably smaller and the labels
above them get truncated ("LOOKA..." instead of "LOOKAHEAD").

Fix: Increase `kControlStripH` and `kKnobRowH` to allow ~44px diameter knobs:
- `kControlStripH`: 92 → 110 (adds 18px to plugin total height → 518px)
- `kKnobRowH`: 56 → 70 (adds 14px to knob row → ~44px knob diameter)
- `kDefaultHeight`: 500 → 518 accordingly

With 44px knobs, labels like "LOOKAHEAD" fit without truncation at font size 9–10.

## Produces
None

## Consumes
None

## Relevant Files
Modify: `M-LIM/src/PluginEditor.h` — `kControlStripH`, `kDefaultHeight` constants (lines 65–66)
Modify: `M-LIM/src/ui/ControlStrip.cpp` — `kKnobRowH` constant (line 6, anonymous namespace)

## Acceptance Criteria
- [ ] Run: `cmake --build M-LIM/build --config Release -j$(nproc)` → Expected: build succeeds
- [ ] Run: launch standalone → Expected: knobs visually larger (~44px diameter), labels "LOOKAHEAD", "ATTACK", "RELEASE" show without truncation, control strip proportionally taller

## Tests
None

## Technical Details

In `M-LIM/src/PluginEditor.h`:
```cpp
static constexpr int kDefaultHeight  = 500;  // → 518
static constexpr int kControlStripH  = 92;   // → 110
```

In `M-LIM/src/ui/ControlStrip.cpp` (anonymous namespace at top):
```cpp
static constexpr int kKnobRowH    = 56;   // → 70
```

With `kKnobRowH = 70`:
- `textHeight = labelH + valueH = 13 + 13 = 26px`
- `knobSize = min(colW, kKnobRowH - textHeight) = min(~139, 70 - 26) = min(139, 44) = 44px`

The `kConstrainer` minimum height in `PluginEditor.cpp`:
```cpp
getConstrainer()->setMinimumSize(600, 350);
```
This doesn't need to change.

Also verify that `kOutputLabelH = 12` and the `totalH` calculation in `ControlStrip::resized()`
still works with the new `kKnobRowH`:
```cpp
const int totalH = kKnobRowH + kPadding + kBtnRowH;  // 70 + 4 + 20 = 94 → fine
```

## Dependencies
None
