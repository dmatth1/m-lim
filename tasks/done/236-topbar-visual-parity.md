# Task 236: Top Bar Visual Parity — Logo, Preset, and Button Styling

## Description
Refine the TopBar component to better match FabFilter Pro-L 2's top bar. Based on reference screenshots:

**Changes needed:**
1. **Logo**: Currently "M-LIM" text. Style it similar to Pro-L 2: bold, white, with the product name in larger text. Consider "M-LIM" with a small subtitle or just clean bold white text at 14-16pt.

2. **Preset navigation area**: The preset label area should have a subtle rounded background (`topBarPresetBackground = 0xff232323`). Add `<` and `>` navigation arrow buttons on either side of the preset name.

3. **A/B and Copy buttons**: Style to match Pro-L 2's button appearance — small rectangular buttons with slight rounded corners, using `buttonBackground` color, text in `textSecondary` when inactive.

4. **Top bar background**: Use a gradient from `0xff252525` (top) to `0xff1F1F1F` (bottom) to give depth, similar to Pro-L 2's top bar treatment.

5. **Separator line**: Draw a 1px separator line at the bottom of the top bar using `panelBorder` color.

6. **Help button**: Pro-L 2 shows a "Help" button on the far right. Add a styled "?" button (`helpButton_`) on the far right of the top bar (non-functional, just visual).

Read TopBar.h and TopBar.cpp before making changes. The existing callbacks (onPresetPrev, onPresetNext, onABToggle, onABCopy, onUndo, onRedo) must continue to work.

## Produces
None

## Consumes
None

## Relevant Files
Read: `src/ui/TopBar.h` — current interface
Read: `src/ui/TopBar.cpp` — current implementation
Modify: `src/ui/TopBar.h` — add helpButton_ if not present
Modify: `src/ui/TopBar.cpp` — update paint(), resized(), and button styling
Read: `src/ui/Colours.h` — color constants
Read: `/reference-docs/reference-screenshots/prol2-main-ui.jpg` — top bar reference

## Acceptance Criteria
- [ ] Run: `cd /workspace/M-LIM && cmake --build build --config Release -j$(nproc) 2>&1 | tail -5` → Expected: exits 0
- [ ] Run: `grep "helpButton\|Help" /workspace/M-LIM/src/ui/TopBar.h` → Expected: at least 1 match
- [ ] Run: `grep "drawHorizontalLine\|drawLine" /workspace/M-LIM/src/ui/TopBar.cpp` → Expected: separator line drawn at bottom

## Tests
None

## Technical Details
- The preset name should be centered in the middle ~30% of the top bar width
- A/B and Copy buttons should be to the right of center
- Logo on far left
- Help button on far right
- Top bar height remains kTopBarH = 30px

## Dependencies
None
