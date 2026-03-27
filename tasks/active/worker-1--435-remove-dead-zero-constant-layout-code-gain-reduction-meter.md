# Task 435: Remove Dead Zero-Constant Layout Code in GainReductionMeter

## Description
`GainReductionMeter.h` defines two constants permanently set to zero:

```cpp
static constexpr int kScaleW   = 0;   // width of dB scale labels (0 = hidden)
static constexpr int kNumericH = 0;   // numeric readout removed (was 16)
```

Every frame, `paint()` and `resized()` compute zero-width/zero-height rectangles and
pass them to `drawScale()` and `drawNumeric()` which immediately return via:
```cpp
if (kScaleW <= 0) return;
if (kNumericH <= 0) return;
```

This is dead code that confuses readers about the actual meter layout.

Remove:
- `kScaleW` and `kNumericH` constants from the header
- `removeFromTop(0)` / `removeFromRight(0)` calls in `resized()` and `paint()`
- The early-out-guarded `drawScale()` and `drawNumeric()` functions (if completely dead)
- All call sites of the removed functions

## Produces
None

## Consumes
None

## Relevant Files
Modify: `src/ui/GainReductionMeter.h` — remove zero constants (~lines 45–46)
Modify: `src/ui/GainReductionMeter.cpp` — remove dead layout calls and guarded functions (~lines 38, 77–79, 154 and drawNumeric/drawScale bodies)

## Acceptance Criteria
- [ ] Run: `grep -n "kScaleW\|kNumericH\|drawScale\|drawNumeric" src/ui/GainReductionMeter.cpp` → Expected: no output (all dead code removed)
- [ ] Run: `cmake --build build --target MLIM_Standalone_Standalone -j$(nproc) 2>&1 | tail -3` → Expected: build succeeds, exit 0

## Tests
None

## Technical Details
Read `GainReductionMeter.h` and `GainReductionMeter.cpp` fully before editing to identify
all usages. Confirm `drawScale()` and `drawNumeric()` are not called from anywhere outside
these two files before removing them.

Build Standalone: `cmake --build build --target MLIM_Standalone_Standalone -j$(nproc)`.

## Dependencies
None
