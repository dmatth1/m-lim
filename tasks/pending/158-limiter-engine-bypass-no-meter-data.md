# Task 158: LimiterEngine bypass mode does not push MeterData — UI meters freeze

## Description
In `LimiterEngine::process()`, the bypass branch returns early without pushing any
`MeterData` to `mMeterFIFO`:

```cpp
// src/dsp/LimiterEngine.cpp ~line 181
if (mBypass.load())
{
    mGRdB.store(0.0f);
    float outL = peakLevel(buffer, 0, numSamples);
    float outR = (numChannels > 1) ? peakLevel(buffer, 1, numSamples) : outL;
    mTruePkL.store(outL);
    mTruePkR.store(outR);
    return;   // ← no mMeterFIFO.push()
}
```

`PluginProcessor::processBlock()` drains `limiterEngine.getMeterFIFO()` to populate
`mProcessorMeterFIFO`. When bypass is engaged the engine FIFO is always empty, so the
processor FIFO receives nothing, and `PluginEditor::timerCallback()` never gets new
`MeterData`. The result is that all UI meters (level bars, GR meter, waveform display,
LUFS readouts) freeze at their last value for the entire time the plugin is bypassed.

Note: `inLevelL` and `inLevelR` (input peak levels) are already computed at the top of
`process()` before the bypass check, so they can be reused.

Fix: populate a `MeterData` struct with the bypass signal levels and push it before
returning:

```cpp
if (mBypass.load())
{
    mGRdB.store(0.0f);
    float outL = peakLevel(buffer, 0, numSamples);
    float outR = (numChannels > 1) ? peakLevel(buffer, 1, numSamples) : outL;
    mTruePkL.store(outL);
    mTruePkR.store(outR);

    MeterData md;
    md.inputLevelL   = inLevelL;
    md.inputLevelR   = inLevelR;
    md.outputLevelL  = outL;
    md.outputLevelR  = outR;
    md.gainReduction = 0.0f;
    md.truePeakL     = outL;
    md.truePeakR     = outR;
    md.waveformSize  = 1;
    md.waveformBuffer[0] = 0.0f;  // zero GR
    mMeterFIFO.push(md);
    return;
}
```

## Produces
None

## Consumes
None

## Relevant Files
Modify: `src/dsp/LimiterEngine.cpp` — add MeterData push in bypass branch (~line 181)
Read: `src/dsp/MeterData.h` — MeterData struct fields
Read: `src/dsp/LimiterEngine.h` — LimiterEngine class overview

## Acceptance Criteria
- [ ] Run: `cd build && ctest --output-on-failure -R MLIMTests` → Expected: all tests pass
- [ ] Run: `grep -A 15 "if (mBypass" src/dsp/LimiterEngine.cpp` → Expected: output shows
  `mMeterFIFO.push` call inside the bypass branch before `return`

## Tests
- Unit: `tests/dsp/test_limiter_engine_modes.cpp` — add a test that enables bypass mode,
  calls `process()` with a non-silent buffer, then pops from `getMeterFIFO()` and verifies:
  (a) at least one MeterData was pushed, (b) `md.gainReduction == 0.0f`,
  (c) `md.inputLevelL > 0.0f` and `md.outputLevelL > 0.0f` (levels match input signal)

## Technical Details
- `inLevelL` / `inLevelR` are computed at the very top of `process()` before the bypass
  check (lines ~175-176); they are local variables and remain valid in the bypass branch
- `MeterData::waveformBuffer[0]` should be `0.0f` (no gain reduction in bypass)
- The LUFS fields (`momentaryLUFS` etc.) can be left at their zero-default values;
  `PluginProcessor::processBlock()` still calls `loudnessMeter.processBlock()` after
  `limiterEngine.process()` returns, so it will fill in accurate LUFS when it augments
  the MeterData

## Dependencies
None
