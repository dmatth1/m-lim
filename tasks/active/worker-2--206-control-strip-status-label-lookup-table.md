# Task 206: Replace ControlStrip Status-Label Switch Statements with Lookup Arrays

## Description
`ControlStrip::updateStatusLabels()` contains two `switch` statements that map combo-box indices to display strings:

```cpp
switch (oversamplingBox_.getSelectedId()) {
    case 1: oversamplingLabel_.setText("Oversampling: Off", ...); break;
    case 2: oversamplingLabel_.setText("Oversampling: 2x", ...); break;
    // ...
}
switch (ditherBox_.getSelectedId()) {
    case 1: ditherLabel_.setText("Dither: Off", ...); break;
    // ...
}
```

This pattern is brittle: adding a new oversampling option requires editing three places (the combo box population, the switch case, and probably a test). Replace both switch statements with `static constexpr` string arrays (one per parameter), indexed by `getSelectedId() - 1`:

```cpp
static constexpr const char* kOversamplingLabels[] = {
    "Oversampling: Off", "Oversampling: 2x", "Oversampling: 4x", ...
};
oversamplingLabel_.setText(kOversamplingLabels[oversamplingBox_.getSelectedId() - 1], ...);
```

Apply the same pattern to `LoudnessPanel::targetChoiceToLUFS()` if it uses a switch for the same reason.

## Produces
None

## Consumes
None

## Relevant Files
Read: `M-LIM/src/ui/ControlStrip.h` — understand the member variables
Modify: `M-LIM/src/ui/ControlStrip.cpp` — replace switch statements in updateStatusLabels()
Read: `M-LIM/src/ui/LoudnessPanel.cpp` — check targetChoiceToLUFS() for same pattern
Modify: `M-LIM/src/ui/LoudnessPanel.cpp` — replace switch if present (optional, same task)

## Acceptance Criteria
- [ ] Run: `grep -c "switch.*getSelectedId\|case [0-9].*setText" M-LIM/src/ui/ControlStrip.cpp` → Expected: 0 (switch replaced)
- [ ] Run: `cmake --build build -j4 2>&1 | tail -5` → Expected: build succeeds, exit 0

## Tests
None

## Technical Details
- Array must be `static constexpr` inside the function or as a file-scope constant — not heap-allocated.
- Add a bounds check (assert or clamp) before indexing: `jassert(idx >= 0 && idx < numLabels)`.
- Keep the display strings identical to those in the current switch cases — this is a pure refactor, not a feature change.
- If `LoudnessPanel::targetChoiceToLUFS()` is a switch mapping index → float value, it can use a similar `static constexpr float kTargetLUFS[]` array.

## Dependencies
None
