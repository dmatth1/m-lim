# Task 233: Control Strip — Make Channel Linking Always Visible

## Description
In FabFilter Pro-L 2, the CHANNEL LINKING section (with TRANSIENTS and RELEASE knobs) is always visible in the control strip — it does NOT require clicking an ADVANCED button to reveal. Currently in M-LIM, these knobs are hidden behind the ADVANCED toggle.

Changes needed in ControlStrip:
1. Make `channelLinkTransientsKnob_` and `channelLinkReleaseKnob_` always visible (use `addAndMakeVisible` instead of `addChildComponent` for them)
2. Remove the ADVANCED button's behavior of toggling their visibility (keep the button for any other purpose it may have, or rename to something else)
3. Update `resized()` to always allocate space for both channel linking knobs between the RELEASE knob and the right edge (before the outputCeiling slider)
4. Add a "CHANNEL LINKING" section label above the two channel linking knobs (drawn in `paint()`, centered above the knob pair area)
5. The `isAdvancedExpanded_` flag can remain for any other uses, but channel linking visibility must not depend on it

Layout in resized() after this change:
- Left: STYLE selector (2 knob-widths)
- Then: LOOKAHEAD, ATTACK, RELEASE (1 knob-width each)
- Then: CHANNEL LINKING area (2 knob-widths — TRANSIENTS + RELEASE knobs)
- Then: ADVANCED button (fixed width)
- Right: OUTPUT CEILING vertical slider

Update the "CHANNEL LINKING" painted label in paint() to always draw (not conditionally on isAdvancedExpanded_).

## Produces
None

## Consumes
None

## Relevant Files
Modify: `src/ui/ControlStrip.cpp` — change visibility, update resized(), update paint()
Modify: `src/ui/ControlStrip.h` — update isAdvancedExpanded_ usage if needed
Read: `/reference-docs/reference-screenshots/prol2-intro.jpg` — shows channel linking always visible

## Acceptance Criteria
- [ ] Run: `grep "addAndMakeVisible.*channelLink" /workspace/M-LIM/src/ui/ControlStrip.cpp` → Expected: 2 matches (both knobs always visible)
- [ ] Run: `grep "addChildComponent.*channelLink" /workspace/M-LIM/src/ui/ControlStrip.cpp` → Expected: 0 matches
- [ ] Run: `cd /workspace/M-LIM && cmake --build build --config Release -j$(nproc) 2>&1 | tail -5` → Expected: exits 0

## Tests
None

## Technical Details
- kNumKnobs constant in the anonymous namespace may need updating (currently 7 — may need to be 9 to include both channel link knobs in the width calculation)
- The CHANNEL LINKING label should be drawn above both knobs together (centered over the combined area)
- The ADVANCED button should remain but only control any remaining hidden controls (dither, oversampling UI elements if any)

## Dependencies
None
