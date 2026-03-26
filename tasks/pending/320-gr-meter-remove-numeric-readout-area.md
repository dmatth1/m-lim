# Task 320: GR Meter — Remove Cramped Numeric Readout Area

## Description
The `GainReductionMeter` reserves 16px at the top (`kNumericH = 16`) for a numeric readout
displaying "0.0" (current GR) and "0.0" (peak GR) at all times. At rest with no audio,
this shows a cramped "0.0 / 0.0" text pair at the top of a component that is only 12px wide
(`kGRMeterW = 12` in PluginEditor).

The Pro-L 2 reference shows a clean, uninterrupted GR bar with no such numeric area at the
top. The cramped text is not legible at 12px width and adds visual clutter at the top of the
right panel (visible in the screenshot as "0.0" text above the output meter scale).

Fix: Remove the numeric readout from the GR bar completely by setting `kNumericH = 0`.
The peak GR value is less important than a clean visual since the full GR history is visible
in the waveform display's GR overlay.

## Produces
None

## Consumes
None

## Relevant Files
Modify: `M-LIM/src/ui/GainReductionMeter.h` — `kNumericH` constant, line 46
Modify: `M-LIM/src/ui/GainReductionMeter.cpp` — `peakLabelArea()` and `paint()` which use `kNumericH`

## Acceptance Criteria
- [ ] Run: `cmake --build M-LIM/build --config Release -j$(nproc)` → Expected: build succeeds
- [ ] Run: launch standalone → Expected: no "0.0"/"0.0" text visible at top of GR meter column; GR bar fills the full component height from top to bottom

## Tests
None

## Technical Details
In `GainReductionMeter.h`, change:
```cpp
static constexpr int kNumericH = 16;
```
to:
```cpp
static constexpr int kNumericH = 0;
```

The `paint()` method does `bounds.removeFromTop(kNumericH)` and calls `drawNumeric()`.
With `kNumericH = 0`, `removeFromTop(0)` is a no-op, `drawNumeric()` will be called with
an empty rect and draw nothing visible.

Also update `peakLabelArea()` to return an empty rectangle when `kNumericH == 0` so
mouse-click-to-reset-peak still works (or just guard with if kNumericH > 0).

The `mouseDown()` handler calls `peakLabelArea().contains(e.position)` — this will simply
never be true with a zero-height area, which is fine (peak reset can be removed or kept as dead code).

## Dependencies
None
