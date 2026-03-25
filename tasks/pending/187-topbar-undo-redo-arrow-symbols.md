# Task 187: TopBar — Undo/Redo Buttons Use Arrow Symbols Instead of Text Labels

## Description
The `TopBar` declares undo/redo as text-labelled buttons:
```cpp
juce::TextButton undoButton_ { "Undo" };
juce::TextButton redoButton_ { "Redo" };
```
At the current button width (`kBtnW = 38` px), these labels truncate to "U..." and "Re..." in the rendered UI (confirmed via screenshot of the running plugin).

The reference Pro-L 2 uses **small arrow/arrow-pair symbols** (↩ / ↪ or similar compact glyphs) for the undo/redo controls, allowing them to be visually clear at small sizes.

Reference: `/reference-docs/reference-screenshots/prol2-intro.jpg` — top-left area of the plugin shows compact undo/redo controls using arrow symbols.

Fix: Replace the text labels with Unicode arrow characters that render clearly at small sizes:
- Undo: `"↩"` (U+21A9 LEFTWARDS ARROW WITH HOOK) or `"◀"`
- Redo: `"↪"` (U+21AA RIGHTWARDS ARROW WITH HOOK) or `"▶"`

Or alternatively: keep the text "Undo"/"Redo" but widen the buttons (increase `kBtnW` to 48 or more in `TopBar.cpp`). The arrow symbol approach is preferred for visual parity.

## Produces
None

## Consumes
None

## Relevant Files
Modify: `M-LIM/src/ui/TopBar.h` — update `undoButton_` and `redoButton_` default labels
Modify: `M-LIM/src/ui/TopBar.cpp` — optionally adjust `kBtnW` if needed
Read: `/reference-docs/reference-screenshots/prol2-intro.jpg` — visual reference

## Acceptance Criteria
- [ ] Run: `cmake --build build --config Release -j$(nproc) && echo OK` → Expected: `OK`
- [ ] Run: build plugin, screenshot top bar — expected: undo/redo buttons show arrow symbols (not truncated text) at their current button width
- [ ] Run: `cd build && ctest --output-on-failure` → Expected: all tests pass, including any TopBar tests

## Tests
None (visual-only change)

## Technical Details
In `M-LIM/src/ui/TopBar.h`, change button initialisers:
```cpp
// BEFORE:
juce::TextButton undoButton_ { "Undo" };
juce::TextButton redoButton_ { "Redo" };

// AFTER:
juce::TextButton undoButton_ { juce::CharPointer_UTF8 ("\xe2\x86\xa9") };  // ↩
juce::TextButton redoButton_ { juce::CharPointer_UTF8 ("\xe2\x86\xaa") };  // ↪
```

Alternatively, use simpler arrows:
```cpp
juce::TextButton undoButton_ { "<" };   // or "◀" / "⟵"
juce::TextButton redoButton_ { ">" };   // or "▶" / "⟶"
```

The simplest approach that removes the truncation: just use `"<"` / `">"` and keep `kBtnW = 38`. Or use the same Unicode approach already used for `waveformModeButton_` (which uses `juce::CharPointer_UTF8("\xe2\x89\x8b")` for ≋).

## Dependencies
None
