# Task 173: GainReductionMeter — Move Peak/Current Readout to Top of Meter Column

## Description
The `GainReductionMeter::drawNumeric()` currently places the current GR value and peak GR value at the **bottom** of the meter column. In the reference Pro-L 2, peak level readouts appear at the **top** of the metering columns.

Current behaviour: "0.0" (red, current GR) and "0.0" (yellow, peak GR) in a small strip at the bottom of the component.

Reference (`prol2-main-ui.jpg`, `prol2-intro.jpg`): small numeric readouts visible at the **top** of the metering panel, above the bar area — serving as "header" indicators for the peak value held.

Additionally, the GR readout area currently occupies `kNumericH` pixels at the bottom of the component, leaving the bar scaled to the remaining height. Moving the readout to the top requires swapping the layout in `paint()`:
```cpp
// BEFORE (current):
auto numArea = bounds.removeFromBottom(kNumericH);
auto barArea = bounds; (after scale removal)

// AFTER (proposed):
auto numArea = bounds.removeFromTop(kNumericH);
auto barArea = bounds; (after scale removal)
```

## Produces
None

## Consumes
None

## Relevant Files
Modify: `M-LIM/src/ui/GainReductionMeter.cpp` — swap `removeFromBottom` to `removeFromTop` for `numArea` in `paint()`, update `peakLabelArea()` accordingly
Read: `/reference-docs/reference-screenshots/prol2-main-ui.jpg` — visual reference
Read: `M-LIM/src/ui/GainReductionMeter.h` — check `peakLabelArea()` helper (used for mouse hit testing)

## Acceptance Criteria
- [ ] Run: `cmake --build build --config Release -j$(nproc) && echo OK` → Expected: `OK`
- [ ] Run: build plugin, screenshot — expected: GR numeric readout ("0.0" / "0.0") appears at the TOP of the GR meter column, not the bottom
- [ ] Run: `cd build && ctest --output-on-failure` → Expected: all tests pass

## Tests
None

## Technical Details
In `GainReductionMeter::paint()` (`GainReductionMeter.cpp`, line ~72):
```cpp
// BEFORE:
auto numArea   = bounds.removeFromBottom (static_cast<float> (kNumericH));
auto scaleArea = bounds.removeFromRight  (static_cast<float> (kScaleW));

// AFTER:
auto numArea   = bounds.removeFromTop    (static_cast<float> (kNumericH));
auto scaleArea = bounds.removeFromRight  (static_cast<float> (kScaleW));
```

Also update `GainReductionMeter::peakLabelArea()` to match — it currently calculates the peak label area relative to the bottom, so after the layout swap it will need to reference the top:
```cpp
// BEFORE:
auto numArea = bounds.removeFromBottom(kNumericH);
auto cur = numArea.withHeight(numArea.getHeight() * 0.5f);
return cur.withY(cur.getBottom());

// AFTER:
auto numArea = bounds.removeFromTop(kNumericH);
return numArea.withHeight(numArea.getHeight() * 0.5f).translated(0, numArea.getHeight() * 0.5f);
```

## Dependencies
None
