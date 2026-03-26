# Task 331: Waveform Gradient Colors — Brighten Bottom to Match Reference

## Description
The waveform display gradient bottom color is too dark and insufficiently blue compared to
the reference. Pixel sampling at y=370-380 (near-bottom of waveform area, where background
is visible without audio data) in the 900x500 reference shows:

  Reference at y=370-380, x=100-600: ~#7A809B to #7F84A2 (R=122-127, G=128-132, B=155-162)

Our current `displayGradientBottom = #506090` renders as:
  M-LIM at y=370: #50608E (R=80, G=96, B=142) — measured

The reference bottom color is ~40 units brighter in R, ~32 units brighter in G, ~14 units
brighter in B. The reference background is a lighter, softer steel-blue.

Cross-check from v1-0005.png (Pro-L 2 waveform closeup, background visible in middle):
  y=5  (top): #99A9CC — lighter blue
  y=200 (mid): #6F7790 — medium steel-blue
  y=280 (bottom): #565E76 — darker blue-gray

The Colours.h comment cites "top ~#8992AB, middle ~#6F7790" from v1-0005.png, confirmed.

**Recommended change**: Brighten displayGradientBottom toward the reference's bottom color.
The measured reference bottom (#7A809B) is lighter than v1-0005 bottom (#565E76), suggesting
the waveform in the main reference shows a blend of background + output waveform content at
the bottom. A reasonable target is #687090, which represents a midpoint between v1-0005
measured bottom (#565E76) and the reference prol2-main-ui.jpg bottom (#7A809B).

Also consider slightly brightening and bluing the gradient top to better match the reference's
characteristic steel-blue gradient. The v1-0005.png shows top ~#99A9CC.

## Produces
None

## Consumes
None

## Relevant Files
Modify: `M-LIM/src/ui/Colours.h` — adjust `displayGradientTop` and `displayGradientBottom`

## Acceptance Criteria
- [ ] Run: `cmake --build M-LIM/build --config Release -j$(nproc) 2>&1 | tail -3` → Expected: build succeeds, exit 0
- [ ] Run: `source Scripts/ui-test-helper.sh && start_app && screenshot "screenshots/task-331-after.png" && stop_app` → Expected: screenshot saved
- [ ] Visual: Waveform background gradient is lighter steel-blue at the bottom, creating more visible contrast with the top

## Tests
None

## Technical Details
In `M-LIM/src/ui/Colours.h`, change the gradient color constants:

```cpp
// BEFORE:
// Reference samples from Pro-L 2: top ~#8992AB, middle ~#6F7790 (measured from v1-0005.png)
const juce::Colour displayGradientTop   { 0xff686468 };  // neutral/warm gray, matches ref top
const juce::Colour displayGradientBottom{ 0xff506090 };  // more blue-saturated, matches ref center

// AFTER (calibrated from prol2-main-ui.jpg 900x500 comparison + v1-0005.png):
// Top from v1-0005: ~#8992AB. Bottom from main ref: ~#7A809B (near bottom of waveform where bg visible)
const juce::Colour displayGradientTop   { 0xff7880A0 };  // steel-blue, matches Pro-L 2 top area
const juce::Colour displayGradientBottom{ 0xff6878A8 };  // brighter steel-blue, closer to reference bottom
```

Worker note: The exact values should be tuned toward:
- Top: somewhere between current #686468 and reference #8992AB
  → try #7880A0 (bluer, slightly lighter)
- Bottom: toward #7A809B (reference bottom) from current #506090
  → try #6878A8 (lighter, more saturated blue)

After making changes, run an RMSE measurement to confirm improvement before committing.
If RMSE gets worse, try different intermediate values or revert this task entirely — the
existing task-305/316 gradient calibration gave 20.00% waveform RMSE and that was a good result.

## Dependencies
Requires tasks 328 and 329 to be complete (layout revert) so that the waveform area has
the same height/width as when the 20.00% RMSE was measured, making comparison meaningful.
