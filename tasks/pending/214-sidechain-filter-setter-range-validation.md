# Task 214: Add Range Clamping to SidechainFilter Setters

## Description
`SidechainFilter` has three parameter setters — `setHighPassFreq()`, `setLowPassFreq()`, and `setTilt()` — that accept raw float values without any validation or clamping. If the UI or a host passes an out-of-range value, the bilinear transform coefficient computation can produce NaN or unstable filter coefficients, causing audio artifacts or silence.

By contrast, `TransientLimiter` and `LevelingLimiter` already use `std::clamp()` in their setters.

Add `std::clamp()` calls to all three setters in `SidechainFilter`, using the valid physical ranges:
- HP frequency: 20 Hz – 2000 Hz
- LP frequency: 2000 Hz – 20000 Hz
- Tilt: -6 dB – +6 dB

## Produces
None

## Consumes
None

## Relevant Files
Read: `M-LIM/src/dsp/SidechainFilter.h` — find setter declarations
Modify: `M-LIM/src/dsp/SidechainFilter.cpp` — add `std::clamp()` at top of each setter body

## Acceptance Criteria
- [ ] Run: `grep -A4 "setHighPassFreq\|setLowPassFreq\|setTilt" M-LIM/src/dsp/SidechainFilter.cpp` → Expected: each setter body contains a `std::clamp` call
- [ ] Run: `cmake --build build -j4 2>&1 | tail -5` → Expected: build succeeds, exit 0
- [ ] Run: `cd build && ctest -R sidechain --output-on-failure 2>&1 | tail -10` → Expected: all sidechain filter tests pass

## Tests
Unit: `tests/dsp/test_sidechain_filter.cpp` — add tests that call setters with out-of-range values (e.g., `setHighPassFreq(-1.0f)`, `setHighPassFreq(99999.0f)`) and verify the clamped value is applied (no crash, coefficients are finite)

## Technical Details
```cpp
void SidechainFilter::setHighPassFreq(float hz) {
    hz = std::clamp(hz, 20.0f, 2000.0f);
    // ... existing logic
}
void SidechainFilter::setLowPassFreq(float hz) {
    hz = std::clamp(hz, 2000.0f, 20000.0f);
    // ... existing logic
}
void SidechainFilter::setTilt(float dB) {
    dB = std::clamp(dB, -6.0f, 6.0f);
    // ... existing logic
}
```

## Dependencies
None
