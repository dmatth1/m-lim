# Task 024: Top Bar Component

## Description
Create the top toolbar containing preset navigation, A/B comparison toggle, copy A/B, and undo/redo buttons.

## Produces
Implements: `TopBarInterface`

## Consumes
ColoursDefinition

## Relevant Files
Create: `M-LIM/src/ui/TopBar.h` — class declaration
Create: `M-LIM/src/ui/TopBar.cpp` — implementation
Read: `M-LIM/src/ui/Colours.h` — color constants
Read: `SPEC.md` — TopBarInterface

## Acceptance Criteria
- [ ] Run: `cd M-LIM && cmake --build build --target MLIM_VST3 -j$(nproc) 2>&1 | tail -3` → Expected: builds without errors

## Tests
None (visual component)

## Technical Details
- Layout: horizontal bar at top of plugin window
- Left section: "M-LIM" logo text in accent blue
- Center section: prev arrow | preset name display | next arrow
- Right section: A/B toggle button | Copy A→B button | Undo button | Redo button
- All buttons: dark background, light text, subtle hover highlight
- Preset name: editable text field or label
- Callbacks: onUndo, onRedo, onABToggle, onABCopy, onPresetPrev, onPresetNext
- Height: ~30px, full width of plugin

## Dependencies
Requires task 003
