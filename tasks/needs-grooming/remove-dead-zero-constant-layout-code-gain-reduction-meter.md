# Task: Remove dead zero-constant layout code in GainReductionMeter

## Description
`GainReductionMeter.h` defines two constants that are permanently set to zero:

```cpp
static constexpr int kScaleW   = 0;   // width of dB scale labels (0 = hidden)
static constexpr int kNumericH = 0;   // numeric readout removed (was 16)
```

Every frame, `paint()` and `resized()` compute zero-width/zero-height rectangles
and pass them to `drawScale()` and `drawNumeric()` which do nothing with them
(guarded by `if (kScaleW <= 0) return;` and similar). This is dead code that
confuses readers about the meter's actual layout.

Files affected:
- `src/ui/GainReductionMeter.h` lines 45–46
- `src/ui/GainReductionMeter.cpp` lines 38, 77–79, 154 and all of `drawNumeric()` / `drawScale()`

## Produces
None

## Consumes
None

## Relevant Files
Modify: `src/ui/GainReductionMeter.h` — remove or document the zero constants
Modify: `src/ui/GainReductionMeter.cpp` — remove the dead removeFromTop(0) / removeFromRight(0) calls and the early-out-guarded functions if fully dead
Read: `src/ui/GainReductionMeter.h` — understand full interface before removing

## Acceptance Criteria
- [ ] Run: `cmake --build build --target MLIM_Standalone_Standalone -j$(nproc) 2>&1 | tail -3` → Expected: build succeeds, exit 0
- [ ] Run: `grep "kScaleW\|kNumericH\|drawNumeric\|drawScale" M-LIM/src/ui/GainReductionMeter.cpp | grep -v "^\s*//"` → Expected: no remaining calls to removed functions; or if the functions are kept as stubs, zero calls with a 0-size rect argument

## Tests
None

## Technical Details
Three safe options in order of preference:
1. Remove the constants entirely; remove the `removeFromTop(0)` / `removeFromRight(0)` calls; remove `drawNumeric()` and `drawScale()` if they are fully unreachable.
2. Keep the constants as a documented extension point but add `static_assert(kScaleW == 0 && kNumericH == 0, "enable layout code if re-enabling scale/numeric");` so future readers know this is intentional.
3. If `peakLabelArea()` relies on a non-zero `kNumericH` to place the label, verify the current zero value is the correct choice and add a comment.

No behaviour change — purely a code-quality / dead-code-removal fix.

## Dependencies
None
