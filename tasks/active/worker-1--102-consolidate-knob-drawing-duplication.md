# Task 102: Consolidate Duplicated Knob Drawing Code

## Description
Knob rendering logic is implemented twice in two different places, producing
the exact same visual output via separate code paths:

1. `RotaryKnob::paint()` (`RotaryKnob.cpp` lines 16–127): draws face, graduation
   ticks, track arc, value arc, and pointer tick entirely from scratch using
   hard-coded angles (`kRotaryStartAngle`/`kRotaryEndAngle`).

2. `MLIMLookAndFeel::drawRotarySlider()` (`LookAndFeel.cpp` lines 51–131): draws
   the same elements (face, graduation ticks, track arc, value arc, pointer) when
   any plain `juce::Slider` with `RotaryStyle` uses this LookAndFeel.

These two implementations are structurally identical (same tick count, same
proportions, same colours) but differ in small ways (e.g., `faceRadius` is
`radius * 0.78f` in RotaryKnob vs `radius * 0.85f` in LookAndFeel; pointer
length differs). This means any visual tweak (like the arc range fix in task 044)
must be applied to both files or the two knob types will look inconsistent.

**Root cause**: `RotaryKnob` bypasses LookAndFeel entirely — it draws itself
in `paint()` rather than delegating to `LookAndFeel::drawRotarySlider`. The
internal `slider` in RotaryKnob is rendered via `addAndMakeVisible` and
`setBounds`, but `RotaryKnob::paint()` overdraw the component's bounds manually.

**Fix**: Delete the duplicate drawing code in `RotaryKnob::paint()` and instead
rely on the `LookAndFeel::drawRotarySlider` to render the internal `slider`.
`RotaryKnob::paint()` then only draws the label and value text below the knob.
Alternatively (simpler), eliminate `MLIMLookAndFeel::drawRotarySlider()` and keep
the bespoke `RotaryKnob::paint()` implementation as the single source of truth,
making it clear that `RotaryKnob` is always used instead of plain sliders for
rotary controls.

The **recommended approach** is Option B (keep RotaryKnob::paint, delete
LookAndFeel::drawRotarySlider) because `RotaryKnob` is always used for knobs
and the LookAndFeel version is dead code for all current rotary controls.
Then reconcile the small visual differences (faceRadius, pointer length) so there
is one canonical set of values.

## Produces
None

## Consumes
None

## Relevant Files
Modify: `M-LIM/src/ui/LookAndFeel.cpp` — remove `drawRotarySlider` implementation
  (lines 51–131); confirm no existing UI component other than RotaryKnob uses a
  raw `juce::Slider` with RotaryStyle and relies on this override.
Modify: `M-LIM/src/ui/LookAndFeel.h` — remove `drawRotarySlider` declaration.
Modify: `M-LIM/src/ui/RotaryKnob.cpp` — reconcile any remaining small differences
  (faceRadius: 0.78 vs 0.85; pointer length: faceRadius*0.55 vs *0.60).

## Acceptance Criteria
- [ ] Run: `grep -n "drawRotarySlider" M-LIM/src/ui/LookAndFeel.h M-LIM/src/ui/LookAndFeel.cpp` → Expected: no output (method removed).
- [ ] Run: `grep -rn "drawRotarySlider" M-LIM/src/` → Expected: no output.
- [ ] Run: `cd M-LIM && cmake --build build -j$(nproc) 2>&1 | tail -3` → Expected: builds without errors.

## Tests
None (visual component).

## Technical Details
Before removing `drawRotarySlider` from LookAndFeel, verify with grep that
no other file uses a raw `juce::Slider` with `RotaryHorizontalVerticalDrag` style
outside of `RotaryKnob`. If any such slider exists, it will fall back to the
JUCE default LookAndFeel_V4 rotary rendering (acceptable) or should be converted
to use `RotaryKnob` instead.

**faceRadius reconciliation (REQUIRED):** The two implementations differ in face size:
- `LookAndFeel::drawRotarySlider` uses `faceRadius = radius * 0.85f`
- `RotaryKnob::paint()` uses `faceRadius = radius * 0.78f`

When consolidating (keeping RotaryKnob::paint), ensure `RotaryKnob::paint()` uses
`radius * 0.78f`. The 0.78f ratio leaves visible graduation marks around the edge,
matching Pro-L 2 proportions.

**Note on downstream tasks:** Tasks 084 and 104 both modify `LookAndFeel.cpp` to
update `drawRotarySlider`. If this task (102) runs first, those tasks MUST skip the
`LookAndFeel.cpp` changes (the method will no longer exist) and instead apply the
colour/gradient changes to `RotaryKnob.cpp`'s `paint()` method.

Note: Do NOT start this task until task 044 (fix-knob-arc-range) is complete,
as 044 may alter both files and this task will need to start from the post-044 state.

## Dependencies
Requires task 044
