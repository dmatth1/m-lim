# Task 110: Knob Labels Should Be ALL CAPS to Match Pro-L 2

## Description
In the reference (`prol2-features.jpg`, `prol2-main-ui.jpg`, `v1-0020.png`), all knob labels in the control strip are rendered in **ALL CAPS**: "LOOKAHEAD", "ATTACK", "RELEASE", "STYLE". The current implementation uses title case: "Lookahead", "Attack", "Release".

This is a direct text styling mismatch. The fix is straightforward — change the label strings passed to `setLabel()` in `ControlStrip.cpp`.

While there, the algorithm selector area should also show a "STYLE" header label, since the reference shows "STYLE" as the parameter name above or near the dropdown. Currently the combo box shows no label — it just shows the selected algorithm name (e.g., "Transparent").

## Produces
None

## Consumes
None

## Relevant Files
Modify: `M-LIM/src/ui/ControlStrip.cpp` — change knob label strings to ALL CAPS in constructor (lines ~37-55), add STYLE label to algorithm selector area
Read: `/reference-docs/reference-screenshots/prol2-features.jpg` — shows "HEAD", "ATTACK", "RELEASE", "CHANNEL LINKING" in all caps
Read: `/reference-docs/video-frames/v1-0020.png` — shows "STYLE", "LOOKAHEAD", "ATTACK", "RELEASE" labels

## Acceptance Criteria
- [ ] Run: `grep -A2 'setLabel' M-LIM/src/ui/ControlStrip.cpp` → Expected: all label strings are uppercase ("LOOKAHEAD", "ATTACK", "RELEASE")
- [ ] Run: `cmake --build build -j$(nproc) 2>&1 | tail -3` → Expected: build succeeds

## Tests
None

## Technical Details
Changes needed in `ControlStrip.cpp` constructor:
```cpp
// Before:
lookaheadKnob_.setLabel ("Lookahead");
attackKnob_.setLabel ("Attack");
releaseKnob_.setLabel ("Release");
channelLinkTransientsKnob_.setLabel ("Tr Link");
channelLinkReleaseKnob_.setLabel ("Rel Link");

// After:
lookaheadKnob_.setLabel ("LOOKAHEAD");
attackKnob_.setLabel ("ATTACK");
releaseKnob_.setLabel ("RELEASE");
channelLinkTransientsKnob_.setLabel ("TRANSIENTS");
channelLinkReleaseKnob_.setLabel ("RELEASE");
```

Also add a "STYLE" label drawn in the algorithm selector area. The label should appear above the combo box, small font (9-10px), `textSecondary` colour. This can be done by adding a `juce::Label styleLabel_` member to `ControlStrip` and positioning it above `algorithmSelector_` in `resized()`, or by drawing it in `paint()` above the selector bounds.

## Dependencies
None
