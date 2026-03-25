# Task 142: Fix TopBar "A→B" Button Unicode Rendering

## Description
The A/B copy button in `TopBar.h` is initialized with the Unicode arrow character:
```cpp
juce::TextButton abCopyButton_ { "A→B" };
```
The `→` character (U+2192) can fail to render on some systems/displays, appearing as a replacement character or being clipped. In the current UI screenshot, the button shows as "A(..." — the arrow character is not rendering correctly.

The Pro-L 2 reference uses "Copy" as the label for the A→B copy button (visible in `/reference-docs/video-frames/v1-0020.png`).

Fix: Change the button text from `"A→B"` to `"Copy"` to match Pro-L 2 and use only ASCII characters.

## Produces
None

## Consumes
None

## Relevant Files
Modify: `src/ui/TopBar.h` — change `abCopyButton_` initializer text from `"A→B"` to `"Copy"`

## Acceptance Criteria
- [ ] Run: `source Scripts/ui-test-helper.sh && start_app && screenshot "task-142-after.png" && stop_app` → Expected: screenshot shows "Copy" text clearly rendered in the TopBar A/B copy button, not "A→B" or truncated/garbled text
- [ ] Run: `cmake --build build -j$(nproc) 2>&1 | grep -c error` → Expected: `0`

## Tests
None

## Technical Details
- `src/ui/TopBar.h:44`: `juce::TextButton abCopyButton_ { "A→B" };` → change to `{ "Copy" }`
- The `kBtnWide = 44` px width is sufficient for "Copy" at 12pt font.

## Dependencies
None
