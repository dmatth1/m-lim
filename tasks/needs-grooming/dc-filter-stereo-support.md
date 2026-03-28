# Task: DCFilter processes mono only — no SIMD for stereo hot path

## Description
`DCFilter` in `M-LIM/src/dsp/DCFilter.cpp` processes a single channel at a time. `LimiterEngine` calls it in a per-channel loop:
```cpp
for (int ch = 0; ch < numChannels; ++ch)
    mDCFilters[ch].process(buffer.getWritePointer(ch), numSamples);
```
For stereo (the primary use case), this means two separate passes over different memory regions. A stereo-interleaved or SIMD-parallel DCFilter could process both channels simultaneously, similar to the `LoudnessMeter::Biquad2` pattern that already uses SSE2 to process L/R in parallel.

The DC filter is a first-order IIR: `y[n] = x[n] - x[n-1] + R * y[n-1]`. This has a sequential dependency (y[n] depends on y[n-1]), so it cannot be vectorized across samples. However, it CAN be vectorized across channels — process L and R simultaneously using 2-wide double or 4-wide float SIMD.

## Produces
None

## Consumes
None

## Relevant Files
Modify: `M-LIM/src/dsp/DCFilter.h` — add stereo processing method or refactor to process 2 channels
Modify: `M-LIM/src/dsp/DCFilter.cpp` — implement SIMD stereo path
Read: `M-LIM/src/dsp/LoudnessMeter.h` — `Biquad2` struct as a reference pattern for stereo SIMD

## Acceptance Criteria
- [ ] Run: `cd build && ctest --output-on-failure -R dc_filter` → Expected: all tests pass, exit 0
- [ ] Run: `cd build && ctest --output-on-failure` → Expected: all tests pass, exit 0

## Tests
- Unit: `tests/dsp/test_dc_filter.cpp::test_stereo_parity` — verify stereo SIMD path produces identical output to two separate mono passes

## Technical Details
Low priority optimization. The DC filter runs at the original sample rate (not oversampled) and is a very cheap operation. The benefit is marginal unless the host buffer size is very large. Consider only if profiling shows this as a bottleneck.

## Dependencies
None
