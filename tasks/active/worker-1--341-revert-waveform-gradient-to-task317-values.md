# Task 341: Revert Waveform Gradient to Task-317 Values

## Description
Task 330 changed the waveform gradient from the task-317 values to new values based on
reference pixel sampling. However, this actually worsened the waveform RMSE:
- Task 317 (gradient #686468 top, #506090 bottom): waveform RMSE = 20.00%
- Task 339 (gradient #8992AB top, #687090 bottom after 330+336): waveform RMSE = 23.57%

The fundamental reason: the reference prol2-main-ui.jpg shows ACTIVE AUDIO, so the visible
"background" color is a composite of background gradient + waveform fill layers. Task 330
incorrectly sampled the pure background color from reference instead of the composite appearance.

The task-317 gradient (#686468 top, #506090 bottom) happened to approximate the composite
appearance of the reference waveform region much better (20.00% vs 23.57%).

**Fix**: Revert `displayGradientTop` to `0xff686468` and `displayGradientBottom` to `0xff506090`.

## Produces
None

## Consumes
None

## Relevant Files
Modify: `M-LIM/src/ui/Colours.h` — revert displayGradientTop and displayGradientBottom

## Acceptance Criteria
- [ ] Run: `cmake --build M-LIM/build -j$(nproc) 2>&1 | grep -c "error:"` → Expected: `0`
- [ ] Run: visual inspection — waveform background should be a dark neutral/warm gray fading to
  a blue-saturated gray (darker overall than current)

## Tests
None

## Technical Details
In `M-LIM/src/ui/Colours.h`, change:
```cpp
// CURRENT (from tasks 330+336 — causes 23.57% waveform RMSE):
const juce::Colour displayGradientTop   { 0xff8992AB };
const juce::Colour displayGradientBottom{ 0xff687090 };

// REVERT TO (task-317 values — gave 20.00% waveform RMSE):
const juce::Colour displayGradientTop   { 0xff686468 };
const juce::Colour displayGradientBottom{ 0xff506090 };
```

Also update the comment on the gradient lines to reflect the actual rationale:
```cpp
// Waveform display gradient colours
// These approximate the composite appearance of Pro-L 2 waveform with active audio.
// Reference background pure color is lighter (~#8992AB) but the composite with
// input/output waveform fills appears as a darker neutral-warm gray at top
// and blue-saturated gray at bottom. Task-317 values gave 20.00% waveform RMSE.
const juce::Colour displayGradientTop   { 0xff686468 };  // neutral/warm gray, best composite match
const juce::Colour displayGradientBottom{ 0xff506090 };  // blue-saturated, matches ref composite bottom
```

**ALSO**: The GainReductionMeter uses the waveform gradient as its background (added in task 338).
After this change, the GR meter background will also use these darker values. This is fine —
the GR meter is only 12px wide and its visual blending with the waveform display will still work.

## Dependencies
None (can be done independently from task 340, but both should be done before task 343 remeasure)
