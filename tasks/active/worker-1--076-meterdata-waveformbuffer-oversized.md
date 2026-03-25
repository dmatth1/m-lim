# Task 076: MeterData.waveformBuffer[512] Is Grossly Oversized

## Description
`MeterData` contains a 512-float inline array (`std::array<float, 512>`) for the
waveform snapshot, but `waveformSize` is always set to exactly 1 in the only
place it is produced (`LimiterEngine::process()`, line ~341):

```cpp
md.waveformSize      = 1;
md.waveformBuffer[0] = totalGR;  // GR trace for waveform display
```

A 512-element array is never populated. Each `MeterData` is 512×4 = 2048 bytes
for the array alone; the FIFO holds 32 entries, so 32 × 2048 = **65,536 bytes**
of the FIFO ring buffer is dead weight. Beyond memory, copying a 2 KB struct in
every `push()`/`pop()` call on the audio/UI threads is unnecessary cache pressure.

The correct fix is to shrink `waveformBuffer` to match actual usage. The
`WaveformDisplay` task (task 022, pending) will determine the real required size;
until that task lands the practical maximum is 1. A reasonable pre-sized value
is 64 (covering blocks up to 64 samples before the 60fps UI timer fires at 48kHz).

## Produces
None

## Consumes
None

## Relevant Files
Modify: `M-LIM/src/dsp/MeterData.h` — reduce `std::array<float, 512>` to
  `std::array<float, 64>` (or a named constant `kMaxWaveformSamples = 64`)
  and update the comment.
Read: `M-LIM/src/dsp/LimiterEngine.cpp` — only writes waveformSize=1; no
  other writes to waveformBuffer beyond index 0.

## Acceptance Criteria
- [ ] Run: `grep "waveformBuffer" M-LIM/src/dsp/MeterData.h` → Expected: array size is 64 (not 512).
- [ ] Run: `cd M-LIM && cmake --build build -j$(nproc) 2>&1 | tail -3` → Expected: builds without errors.

## Tests
None — size-only change; LimiterEngine only writes index 0; no other code
reads beyond the first element.

## Technical Details
Replace:
```cpp
std::array<float, 512> waveformBuffer{};  ///< Waveform/GR snapshot for WaveformDisplay
```
With:
```cpp
static constexpr int kMaxWaveformSamples = 64;
std::array<float, kMaxWaveformSamples> waveformBuffer{};  ///< Waveform/GR snapshot for WaveformDisplay
```
If task 022 (WaveformDisplay) later requires more samples, increase the constant then.

## Dependencies
None
