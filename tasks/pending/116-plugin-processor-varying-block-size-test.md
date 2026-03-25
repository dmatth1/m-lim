# Task 116: PluginProcessor Varying Block Size Test

## Description
`MLIMAudioProcessor::prepareToPlay(sampleRate, maxBlockSize)` is called with a maximum
block size, but DAWs may deliver smaller blocks (or odd-sized blocks) during actual playback.
For example: `prepareToPlay(44100, 512)` followed by `processBlock()` with 64-sample,
256-sample, or 1-sample buffers.

All existing `test_plugin_processor.cpp` and `test_processor_stress.cpp` tests use a
**fixed block size equal to maxBlockSize**. None test what happens when the actual block
size is smaller than maxBlockSize.

Potential failure modes:
- Buffer pointers may be valid but smaller blocks may read/write beyond their bounds
- Pre-allocated working buffers (e.g., `mPreLimitBuffer` allocated at `maxBlockSize`) may
  be accessed incorrectly when `numSamples < maxBlockSize`
- Oversampler's internal state may be mishandled for non-maxBlockSize inputs

Add tests to `tests/integration/test_plugin_processor.cpp` that verify:

1. **Smaller-than-max block sizes produce finite, bounded output**: After `prepareToPlay(44100, 512)`,
   call `processBlock()` with buffers of 1, 16, 64, 128, and 256 samples. Output must be
   finite and within ceiling.

2. **Mixed block sizes in sequence**: Alternate between 512 and 64-sample blocks across 50
   total process calls. Output must remain finite and never exceed ceiling.

3. **Block size of 1 sample (extreme edge case)**: `processBlock()` with a 1-sample buffer
   must not crash. Output must be finite.

## Produces
None

## Consumes
None

## Relevant Files
Read: `src/PluginProcessor.h` — prepareToPlay, processBlock signature
Read: `src/dsp/LimiterEngine.h` — process(), pre-allocated buffers
Modify: `tests/integration/test_plugin_processor.cpp` — add varying block size tests

## Acceptance Criteria
- [ ] Run: `cd /workspace/M-LIM/build && ctest -R "PluginProcessor" --output-on-failure` → Expected: all tests pass
- [ ] Run: `cd /workspace/M-LIM/build && ctest --output-on-failure 2>&1 | grep -E "passed|failed"` → Expected: no regressions

## Tests
- Unit: `tests/integration/test_plugin_processor.cpp::test_smaller_block_sizes_no_crash` — process 1, 16, 64, 128, 256-sample blocks after prepareToPlay(44100, 512) → no crash, finite output
- Unit: `tests/integration/test_plugin_processor.cpp::test_mixed_block_sizes_bounded` — alternating 512 and 64-sample blocks over 50 calls → output never exceeds ceiling
- Unit: `tests/integration/test_plugin_processor.cpp::test_single_sample_block` — processBlock with 1-sample buffer → no crash, output finite and bounded

## Technical Details
```cpp
TEST_CASE("test_smaller_block_sizes_no_crash", "[PluginProcessor]")
{
    MLIMAudioProcessor proc;
    proc.prepareToPlay(44100.0, 512);

    juce::MidiBuffer midi;
    for (int blockSize : {1, 16, 64, 128, 256})
    {
        juce::AudioBuffer<float> buf(2, blockSize);
        // fill with loud sine
        for (int ch = 0; ch < 2; ++ch)
            for (int i = 0; i < blockSize; ++i)
                buf.setSample(ch, i, 2.0f * std::sin(6.28f * 440.0f * i / 44100.0f));

        REQUIRE_NOTHROW(proc.processBlock(buf, midi));

        // All output must be finite
        for (int ch = 0; ch < 2; ++ch)
            for (int i = 0; i < blockSize; ++i)
                REQUIRE(std::isfinite(buf.getSample(ch, i)));
    }
}
```

## Dependencies
None
