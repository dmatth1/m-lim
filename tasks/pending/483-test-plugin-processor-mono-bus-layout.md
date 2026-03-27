# Task 483: PluginProcessor — Mono Bus Layout Integration Test

## Description
`isBusesLayoutSupported()` accepts both mono and stereo layouts (line 107 in
PluginProcessor.cpp). While `test_bus_layout_stereo_supported` exists, there is **no
integration test** that actually prepares and processes audio in mono layout. The LimiterEngine
mono tests exist at the engine level, but the full PluginProcessor→LimiterEngine→LoudnessMeter
pipeline in mono mode is untested.

This matters because `prepareToPlay` computes `numChannels = max(input, output)`, and the
entire chain (including the LoudnessMeter and meter FIFO augmentation) must handle 1 channel
without OOB access or incorrect metering.

## Produces
None

## Consumes
None

## Relevant Files
Read: `src/PluginProcessor.cpp` — prepareToPlay, processBlock, isBusesLayoutSupported
Read: `tests/integration/test_plugin_processor.cpp` — existing tests
Modify: `tests/integration/test_plugin_processor.cpp` — add mono integration test

## Acceptance Criteria
- [ ] Run: `cd build && ctest -R "PluginProcessor" --output-on-failure` → Expected: all tests pass, exit 0

## Tests
- Integration: `tests/integration/test_plugin_processor.cpp::test_mono_processblock_no_crash` — create processor with mono layout, prepareToPlay, process 10 blocks of mono sine; no crash, output finite
- Integration: `tests/integration/test_plugin_processor.cpp::test_mono_meter_fifo_populated` — process mono audio; verify meter FIFO contains data and inputLevelL > 0

## Technical Details
JUCE AudioProcessor can be configured for mono via `BusesLayout` with
`AudioChannelSet::mono()` for both input and output. The test should verify
`isBusesLayoutSupported()` returns true for this config, then actually process audio.

## Dependencies
None
