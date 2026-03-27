# Task: Reduce Level Meter Idle Gradient Bottom Alpha

## Description
The level meter idle structural gradient has its BOTTOM stop at
`meterSafe.darker(0.3f).withAlpha(0.44f)`, producing a visible steel-blue/teal glow at
the bottom of the idle meter bars. Task-372 already reduced the TOP zone alpha from 0.44
to 0.10, but the BOTTOM stop was not changed.

In the reference, idle meters (no audio playing) appear near-dark with minimal ambient
color. The current 0.44 alpha bottom stop contributes a noticeable blue-teal ambient fill
to the lower portion of the meter bars, which conflicts with the dark reference appearance.

**Expected RMSE gain:** Right panel −0.3pp.

## Produces
None

## Consumes
None

## Relevant Files
Modify: `src/ui/LevelMeter.cpp` — `drawChannel()` method, idle gradient bottom stop
Read: `src/ui/Colours.h` — `meterSafe`, `barTrackBackground` constants

## Acceptance Criteria
- [ ] Run: build + screenshot + RMSE → Expected: Right panel RMSE ≤ 23.3% (down from 23.57%)
- [ ] Run: visual check of idle meters → Expected: meter bars appear dark at idle, no visible blue glow at bottom
- [ ] Run: Full RMSE → Expected: ≤ 21.1% (no regression)

## Tests
None

## Technical Details

In `src/ui/LevelMeter.cpp`, inside `drawChannel()`, locate the idle gradient definition:

```cpp
juce::ColourGradient idleGrad (
    MLIMColours::meterDanger.withAlpha (0.10f),              0.0f, barTop2,
    MLIMColours::meterSafe.darker (0.3f).withAlpha (0.44f),  0.0f, barTop2 + barH2,
    false);
```

Change the bottom stop alpha from `0.44f` to `0.08f`:

```cpp
juce::ColourGradient idleGrad (
    MLIMColours::meterDanger.withAlpha (0.10f),              0.0f, barTop2,
    MLIMColours::meterSafe.darker (0.3f).withAlpha (0.08f),  0.0f, barTop2 + barH2,
    false);
```

Also reduce the warm extension stop alpha from `0.30f` to `0.08f`:
```cpp
idleGrad.addColour ((warmExtY - barTop2) / barH2,
    MLIMColours::meterSafe.brighter (0.15f).withAlpha (0.08f));  // was 0.30f
```

**Rationale:** With the top zone already at 0.10 alpha (task-372), keeping the bottom
at 0.44 creates a visible gradient that ramps from near-transparent at top to
semi-opaque blue at bottom. Making both ends consistently near-transparent (~0.08)
produces a nearly dark idle state that matches the reference's dark meter appearance.

**Measurement:**
```bash
# Right panel RMSE (x=720-900, full height)
convert /tmp/mlim.png -crop 180x500+720+0 +repage /tmp/right.png
convert /tmp/ref.png  -crop 180x500+720+0 +repage /tmp/ref_right.png
compare -metric RMSE /tmp/ref_right.png /tmp/right.png /dev/null 2>&1
```

## Dependencies
None
