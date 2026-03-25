# Task 144: Simplify MeterData waveformBuffer — only index 0 is ever written

## Description
`MeterData.h` declares a 64-element waveform buffer:
```cpp
static constexpr int kMaxWaveformSamples = 64;
std::array<float, kMaxWaveformSamples> waveformBuffer{};
int waveformSize = 0;
```

But `LimiterEngine::process()` always writes exactly one value:
```cpp
md.waveformSize       = 1;
md.waveformBuffer[0]  = totalGR;
```

This wastes `63 * 4 = 252` bytes per `MeterData` struct in the FIFO (FIFO capacity=64, so
252 * 64 = ~16 KB wasted), and the variable-length protocol (with `waveformSize`) adds complexity
that is never exercised.

If the waveform display only ever receives one GR sample per block, simplify `MeterData` to
use a plain scalar `float waveformSample` (removing `waveformBuffer` and `waveformSize`).
Update `LimiterEngine::process()` and `WaveformDisplay::pushMeterData()` accordingly.

**Before simplifying, verify**: check `WaveformDisplay::pushMeterData()` to confirm it only reads
`waveformBuffer[0]`. If it reads multiple samples, document why and leave the array but add a
comment explaining the intended future use. If it only reads index 0, do the simplification.

## Produces
None

## Consumes
None

## Relevant Files
Modify: `M-LIM/src/dsp/MeterData.h` — replace array+size with scalar `waveformSample`
Modify: `M-LIM/src/dsp/LimiterEngine.cpp` — update MeterData population (lines 332-333)
Modify: `M-LIM/src/ui/WaveformDisplay.cpp` — update `pushMeterData()` to read scalar field
Modify: `M-LIM/src/ui/WaveformDisplay.h` — update `pushMeterData()` signature if needed

## Acceptance Criteria
- [ ] Run: `grep -n "waveformBuffer\|waveformSize\|kMaxWaveformSamples" M-LIM/src/dsp/MeterData.h` → Expected: no output (fields removed)
- [ ] Run: `grep -n "waveformSample" M-LIM/src/dsp/MeterData.h` → Expected: 1 line (new field declaration)
- [ ] Run: `cmake --build build -j$(nproc) 2>&1 | grep -E "error:"` → Expected: no output

## Tests
None

## Technical Details
New field in MeterData:
```cpp
float waveformSample = 0.0f;  ///< GR sample for WaveformDisplay (one per audio block)
```

In LimiterEngine::process():
```cpp
// Remove: md.waveformSize = 1; md.waveformBuffer[0] = totalGR;
md.waveformSample = totalGR;
```

In WaveformDisplay::pushMeterData(), change any reference from
`data.waveformBuffer[0]` to `data.waveformSample`.

## Dependencies
None
