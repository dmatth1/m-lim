# Task 484: PluginProcessor::processBlockBypassed — Latency Maintenance Test

## Description
`processBlockBypassed()` delegates to `processBlock()` to maintain latency compensation
when the host engages its own bypass (line 102 in PluginProcessor.cpp). There is **no test**
for this method. If someone accidentally changes it to pass audio through directly (the
usual AudioProcessor default), delay-compensated tracks in the DAW would go out of sync
during host bypass — a regression that's invisible in unit tests but audible in production.

## Produces
None

## Consumes
None

## Relevant Files
Read: `src/PluginProcessor.cpp` — processBlockBypassed() implementation (~line 94-103)
Read: `src/PluginProcessor.h` — declaration
Modify: `tests/integration/test_plugin_processor.cpp` — add test

## Acceptance Criteria
- [ ] Run: `cd build && ctest -R "PluginProcessor" --output-on-failure` → Expected: all tests pass, exit 0

## Tests
- Integration: `tests/integration/test_plugin_processor.cpp::test_processBlockBypassed_maintains_latency` — set lookahead > 0, call processBlockBypassed with an impulse at sample 0; verify output impulse appears at the expected latency offset (not at sample 0)
- Integration: `tests/integration/test_plugin_processor.cpp::test_processBlockBypassed_populates_meter_fifo` — call processBlockBypassed; verify meter FIFO has data (proves it actually ran processBlock)

## Technical Details
Create processor, prepareToPlay, set lookahead to 2ms. Feed an impulse buffer via
processBlockBypassed. Check that the peak in the output buffer is delayed by the
reported latency samples — proving the lookahead delay path is active even during
host-bypass.

## Dependencies
None
